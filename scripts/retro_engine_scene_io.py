bl_info = {
    "name": "Retro Engine Scene IO",
    "author": "Diego Lopes",
    "version": (2, 0),
    "blender": (5, 0, 0),
    "location": "File > Import-Export",
    "description": "Import and Export Retro Engine scene files (.json) with GLB meshes",
    "category": "Import-Export",
}

import bpy
import json
import os
import shutil
from bpy.props import StringProperty
from bpy_extras.io_utils import ExportHelper, ImportHelper
from mathutils import Matrix, Quaternion, Vector

# ─────────────────────────────────────────────────────────────────
# Constants
# ─────────────────────────────────────────────────────────────────

GROUP_NAME = "RE_MaterialGroup"

LIGHT_TYPE_TO_ENGINE = {
    "POINT": "point",
    "SUN": "directional",
    "SPOT": "spot",
    "AREA": "point",
}
ENGINE_TO_LIGHT_TYPE = {
    "point": "POINT",
    "directional": "SUN",
    "spot": "SPOT",
}

# Basis-change matrix: Blender Z-up  →  Engine Y-up (-Z forward)
_B = Matrix(((1, 0, 0), (0, 0, 1), (0, -1, 0)))
_B_INV = _B.inverted()

TEXTURE_SLOT_MAP = {
    "diffuse": "Diffuse",
    "normal": "Normal",
    "roughness": "Roughness",
    "metallic": "Metallic",
    "emissive": "Emissive",
    "displacement": "Displacement",
}
NON_COLOR_SLOTS = {"normal", "roughness", "metallic", "displacement"}


# ─────────────────────────────────────────────────────────────────
# Coordinate helpers
# ─────────────────────────────────────────────────────────────────

def _pos_to_engine(v):
    """Blender Vector → engine [x, y, z]."""
    return [v.x, v.z, -v.y]


def _pos_to_blender(p):
    """Engine [x, y, z] → Blender Vector."""
    return Vector((p[0], -p[2], p[1]))


def _scale_to_engine(v):
    return [v.x, v.z, v.y]


def _scale_to_blender(s):
    return Vector((s[0], s[2], s[1]))


def _quat_to_engine(q):
    """Blender Quaternion → engine [x, y, z, w]."""
    m = q.to_matrix()
    c = _B @ m @ _B_INV
    cq = c.to_quaternion()
    return [cq.x, cq.y, cq.z, cq.w]


def _quat_to_blender(xyzw):
    """Engine [x, y, z, w] → Blender Quaternion."""
    q = Quaternion((xyzw[3], xyzw[0], xyzw[1], xyzw[2]))
    m = q.to_matrix()
    c = _B_INV @ m @ _B
    return c.to_quaternion()


# ─────────────────────────────────────────────────────────────────
# Custom material node group  (Blender 4.0+ / 5.0+ interface API)
# ─────────────────────────────────────────────────────────────────

def _ensure_node_group():
    """Create *RE_MaterialGroup* if it does not already exist."""
    if GROUP_NAME in bpy.data.node_groups:
        return bpy.data.node_groups[GROUP_NAME]

    ng = bpy.data.node_groups.new(GROUP_NAME, "ShaderNodeTree")

    # --- interface sockets ---
    d = ng.interface.new_socket(name="Diffuse", in_out="INPUT", socket_type="NodeSocketColor")
    d.default_value = (1.0, 1.0, 1.0, 1.0)

    n = ng.interface.new_socket(name="Normal", in_out="INPUT", socket_type="NodeSocketColor")
    n.default_value = (0.5, 0.5, 1.0, 1.0)

    r = ng.interface.new_socket(name="Roughness", in_out="INPUT", socket_type="NodeSocketFloat")
    r.default_value = 1.0
    r.min_value = 0.0
    r.max_value = 1.0

    mt = ng.interface.new_socket(name="Metallic", in_out="INPUT", socket_type="NodeSocketFloat")
    mt.default_value = 0.0
    mt.min_value = 0.0
    mt.max_value = 1.0

    ng.interface.new_socket(name="Emissive", in_out="INPUT", socket_type="NodeSocketColor")

    es = ng.interface.new_socket(name="Emissive Strength", in_out="INPUT", socket_type="NodeSocketFloat")
    es.default_value = 0.0

    ng.interface.new_socket(name="Displacement", in_out="INPUT", socket_type="NodeSocketFloat")

    ng.interface.new_socket(name="Shader", in_out="OUTPUT", socket_type="NodeSocketShader")

    # --- internal nodes ---
    gi = ng.nodes.new("NodeGroupInput")
    gi.location = (-800, 0)

    go = ng.nodes.new("NodeGroupOutput")
    go.location = (400, 0)

    bsdf = ng.nodes.new("ShaderNodeBsdfPrincipled")
    bsdf.location = (-200, 0)
    # Blender 4.0+ renamed Specular → Specular IOR Level
    if "Specular IOR Level" in bsdf.inputs:
        bsdf.inputs["Specular IOR Level"].default_value = 0.0
    elif "Specular" in bsdf.inputs:
        bsdf.inputs["Specular"].default_value = 0.0

    nm = ng.nodes.new("ShaderNodeNormalMap")
    nm.location = (-500, -200)

    # --- links ---
    ng.links.new(gi.outputs["Diffuse"], bsdf.inputs["Base Color"])
    ng.links.new(gi.outputs["Roughness"], bsdf.inputs["Roughness"])
    ng.links.new(gi.outputs["Metallic"], bsdf.inputs["Metallic"])

    em_in = "Emission Color" if "Emission Color" in bsdf.inputs else "Emission"
    ng.links.new(gi.outputs["Emissive"], bsdf.inputs[em_in])
    ng.links.new(gi.outputs["Emissive Strength"], bsdf.inputs["Emission Strength"])

    ng.links.new(gi.outputs["Normal"], nm.inputs["Color"])
    ng.links.new(nm.outputs["Normal"], bsdf.inputs["Normal"])

    ng.links.new(bsdf.outputs["BSDF"], go.inputs["Shader"])

    return ng


# ─────────────────────────────────────────────────────────────────
#  E X P O R T   helpers
# ─────────────────────────────────────────────────────────────────

def _find_group_node(mat):
    if not mat or not mat.use_nodes:
        return None
    for nd in mat.node_tree.nodes:
        if nd.type == "GROUP" and nd.node_tree and nd.node_tree.name == GROUP_NAME:
            return nd
    return None


def _extract_textures(group_node, root_dir):
    models_dir = os.path.join(root_dir, "models")
    os.makedirs(models_dir, exist_ok=True)
    tex = {}
    for inp in group_node.inputs:
        if inp.is_linked:
            src = inp.links[0].from_node
            if src.type == "TEX_IMAGE" and src.image:
                abs_path = bpy.path.abspath(src.image.filepath)
                dst = os.path.join(models_dir, os.path.basename(abs_path))
                rel = os.path.relpath(dst, root_dir).replace("\\", "/")
                tex[inp.name.lower()] = rel
                if os.path.isfile(abs_path):
                    shutil.copy(abs_path, dst)
    return tex


def _export_material(obj, root_dir):
    ret = {
        "diffuseColor": [1.0, 1.0, 1.0, 1.0],
        "roughness": 1.0,
        "metallic": 0.0,
        "emissive": 0.0,
        "castsShadows": False,
        "textures": {},
    }
    if not obj or not obj.data or not obj.data.materials:
        return ret
    mat = obj.data.materials[0]
    gn = _find_group_node(mat)
    if hasattr(mat, "shadow_method"):
        ret["castsShadows"] = mat.shadow_method != "NONE"
    if gn:
        ret["diffuseColor"] = [*gn.inputs["Diffuse"].default_value]
        ret["roughness"] = gn.inputs["Roughness"].default_value
        ret["metallic"] = gn.inputs["Metallic"].default_value
        ret["emissive"] = gn.inputs["Emissive Strength"].default_value
        ret["textures"] = _extract_textures(gn, root_dir)
    return ret


def _export_gltf(obj, root_dir, exported):
    models_dir = os.path.join(root_dir, "models")
    os.makedirs(models_dir, exist_ok=True)
    gltf_path = os.path.join(models_dir, f"{obj.data.name}.glb")
    rel = os.path.relpath(gltf_path, root_dir).replace("\\", "/")
    if rel in exported:
        return rel
    exported.append(rel)

    bpy.ops.object.select_all(action="DESELECT")

    def _hier(o):
        yield o
        for c in o.children:
            yield from _hier(c)

    bpy.context.view_layer.objects.active = obj
    for o in _hier(obj):
        o.select_set(True)

    bpy.ops.export_scene.gltf(
        filepath=gltf_path,
        export_format="GLB",
        use_selection=True,
        export_yup=True,
        export_colors=True,
        export_materials="NONE",
        export_cameras=False,
        export_lights=False,
        export_animations=True,
        export_extras=True,
        export_apply=True,
    )
    return rel


def _export_action(obj):
    action = obj.animation_data.action if obj.animation_data else None
    if not action:
        return {}
    frames = sorted({int(kp.co.x) for fc in action.fcurves for kp in fc.keyframe_points})
    if not frames:
        return None
    data = []
    for f in frames:
        bpy.context.scene.frame_set(f)
        loc, rot, scl = obj.matrix_local.decompose()
        data.append({
            "frame": f,
            "position": _pos_to_engine(loc),
            "rotation": _quat_to_engine(rot),
            "scale": _scale_to_engine(scl),
        })
    return data


def _obj_to_dict(obj, root_dir, exported):
    loc, rot, scl = obj.matrix_local.decompose()
    node = {
        "name": obj.name,
        "type": "model",
        "position": _pos_to_engine(loc),
        "rotation": _quat_to_engine(rot),
        "scale": _scale_to_engine(scl),
        "children": [],
        "animationFPS": bpy.context.scene.render.fps,
        "animation": _export_action(obj),
    }

    # Custom properties → prefixed with $
    for k in obj.keys():
        if not k.startswith("_"):
            node[f"${k}"] = obj[k]

    if obj.type == "LIGHT":
        node["type"] = "light"
        lt = obj.data
        node["color"] = [round(c, 3) for c in lt.color[:]]
        node["radius"] = round(lt.cutoff_distance, 4) if hasattr(lt, "cutoff_distance") else 0.0
        node["intensity"] = lt.energy / 1000.0 if lt.type != "SUN" else lt.energy
        if lt.type == "SPOT":
            node["spot_angle"] = round(lt.spot_size, 6)
        node["kind"] = LIGHT_TYPE_TO_ENGINE.get(lt.type, "point")
        node["shadows"] = lt.use_shadow

    elif obj.type == "CAMERA":
        node["type"] = "camera"
        cam = obj.data
        node["fovy"] = round(cam.angle_y, 6)
        node["znear"] = cam.clip_start
        node["zfar"] = cam.clip_end

    elif obj.type == "MESH":
        node["type"] = "mesh"
        node["path"] = _export_gltf(obj, root_dir, exported)
        node["material"] = _export_material(obj, root_dir)

    for child in obj.children:
        node["children"].append(_obj_to_dict(child, root_dir, exported))

    return node


# ─────────────────────────────────────────────────────────────────
#  I M P O R T   helpers
# ─────────────────────────────────────────────────────────────────

class _MeshCache:
    """Import each GLB only once and hand out copies of the mesh data."""

    def __init__(self):
        self._cache: dict[str, bpy.types.Mesh | None] = {}

    def get(self, filepath: str):
        if filepath in self._cache:
            md = self._cache[filepath]
            return md.copy() if md else None

        existing = set(bpy.data.objects.keys())
        bpy.ops.import_scene.gltf(filepath=filepath)
        new_objs = [bpy.data.objects[n] for n in bpy.data.objects.keys()
                     if n not in existing]

        mesh_data = None
        for o in new_objs:
            if o.type == "MESH" and o.data:
                mesh_data = o.data
                mesh_data.use_fake_user = True
                break

        for o in new_objs:
            bpy.data.objects.remove(o, do_unlink=True)

        self._cache[filepath] = mesh_data
        return mesh_data.copy() if mesh_data else None


def _import_material(obj, mat_data, base_dir):
    """Build a Blender material with the RE_MaterialGroup node group."""
    _ensure_node_group()

    mat = bpy.data.materials.new(name=f"{obj.name}_Material")
    mat.use_nodes = True
    tree = mat.node_tree

    for nd in list(tree.nodes):
        tree.nodes.remove(nd)

    out_node = tree.nodes.new("ShaderNodeOutputMaterial")
    out_node.location = (400, 0)

    grp = tree.nodes.new("ShaderNodeGroup")
    grp.node_tree = bpy.data.node_groups[GROUP_NAME]
    grp.location = (0, 0)

    grp.inputs["Diffuse"].default_value = mat_data.get("diffuseColor", [1, 1, 1, 1])
    grp.inputs["Roughness"].default_value = mat_data.get("roughness", 1.0)
    grp.inputs["Metallic"].default_value = mat_data.get("metallic", 0.0)
    grp.inputs["Emissive Strength"].default_value = mat_data.get("emissive", 0.0)

    tree.links.new(grp.outputs["Shader"], out_node.inputs["Surface"])

    # Textures
    y_offset = 0
    for slot_key, tex_rel in mat_data.get("textures", {}).items():
        full_path = os.path.join(base_dir, tex_rel)
        if not os.path.isfile(full_path):
            continue
        input_name = TEXTURE_SLOT_MAP.get(slot_key)
        if not input_name or input_name not in grp.inputs:
            continue

        tex = tree.nodes.new("ShaderNodeTexImage")
        tex.location = (-400, y_offset)
        y_offset -= 300
        tex.image = bpy.data.images.load(full_path)
        if slot_key in NON_COLOR_SLOTS:
            tex.image.colorspace_settings.name = "Non-Color"
        tree.links.new(tex.outputs["Color"], grp.inputs[input_name])

    if hasattr(mat, "shadow_method"):
        mat.shadow_method = "OPAQUE" if mat_data.get("castsShadows", False) else "NONE"

    obj.data.materials.append(mat)


def _import_animation(obj, keyframes):
    """Create Blender keyframes from the JSON animation array."""
    if not keyframes or not isinstance(keyframes, list):
        return

    obj.animation_data_create()
    action = bpy.data.actions.new(name=f"{obj.name}_Action")
    obj.animation_data.action = action

    for kf in keyframes:
        frame = kf["frame"]
        obj.location = _pos_to_blender(kf["position"])
        obj.rotation_quaternion = _quat_to_blender(kf["rotation"])
        obj.scale = _scale_to_blender(kf["scale"])
        obj.keyframe_insert(data_path="location", frame=frame)
        obj.keyframe_insert(data_path="rotation_quaternion", frame=frame)
        obj.keyframe_insert(data_path="scale", frame=frame)


def _import_object(data, base_dir, collection, mesh_cache, parent=None):
    """Recursively create Blender objects from one JSON node dict."""
    obj_type = data.get("type", "model")
    name = data.get("name", "Object")
    bl_obj = None

    # ── mesh ──────────────────────────────────────────────────
    if obj_type == "mesh":
        mesh_path = data.get("path", "")
        abs_mesh = os.path.join(base_dir, mesh_path)
        mesh_data = mesh_cache.get(abs_mesh) if os.path.isfile(abs_mesh) else None
        if mesh_data:
            bl_obj = bpy.data.objects.new(name, mesh_data)
        else:
            bl_obj = bpy.data.objects.new(name, bpy.data.meshes.new(name))

    # ── camera ────────────────────────────────────────────────
    elif obj_type == "camera":
        cam = bpy.data.cameras.new(name)
        cam.sensor_fit = "VERTICAL"
        cam.angle = data.get("fovy", 0.8)
        cam.clip_start = data.get("znear", 0.1)
        cam.clip_end = data.get("zfar", 100.0)
        bl_obj = bpy.data.objects.new(name, cam)

    # ── light ─────────────────────────────────────────────────
    elif obj_type == "light":
        kind = data.get("kind", "point")
        bl_type = ENGINE_TO_LIGHT_TYPE.get(kind, "POINT")
        light = bpy.data.lights.new(name, bl_type)
        color = data.get("color", [1, 1, 1])
        light.color = color[:3]
        intensity = data.get("intensity", 1.0)
        light.energy = intensity * 1000.0 if bl_type != "SUN" else intensity
        if hasattr(light, "cutoff_distance"):
            light.cutoff_distance = data.get("radius", 10.0)
            light.use_custom_distance = True
        if bl_type == "SPOT" and "spot_angle" in data:
            light.spot_size = data["spot_angle"]
        light.use_shadow = data.get("shadows", True)
        bl_obj = bpy.data.objects.new(name, light)

    # ── fallback (empty) ──────────────────────────────────────
    else:
        bl_obj = bpy.data.objects.new(name, None)

    collection.objects.link(bl_obj)

    if parent:
        bl_obj.parent = parent

    # Transform (local, in engine coords → Blender coords)
    bl_obj.rotation_mode = "QUATERNION"
    bl_obj.location = _pos_to_blender(data.get("position", [0, 0, 0]))
    bl_obj.rotation_quaternion = _quat_to_blender(data.get("rotation", [0, 0, 0, 1]))
    bl_obj.scale = _scale_to_blender(data.get("scale", [1, 1, 1]))

    # Material
    if obj_type == "mesh" and "material" in data:
        _import_material(bl_obj, data["material"], base_dir)

    # Animation
    _import_animation(bl_obj, data.get("animation"))

    # Custom properties (keys prefixed with $)
    for k, v in data.items():
        if k.startswith("$"):
            bl_obj[k[1:]] = v

    # Recurse children
    for child in data.get("children", []):
        _import_object(child, base_dir, collection, mesh_cache, parent=bl_obj)

    return bl_obj


# ─────────────────────────────────────────────────────────────────
# Operators
# ─────────────────────────────────────────────────────────────────

class RETRO_OT_export_scene(bpy.types.Operator, ExportHelper):
    """Export visible root objects to a Retro Engine JSON scene"""
    bl_idname = "export_scene.retro_engine"
    bl_label = "Export Retro Engine Scene"
    filename_ext = ".json"

    filter_glob: StringProperty(default="*.json", options={"HIDDEN"}, maxlen=255)

    def execute(self, context):
        root_dir = os.path.dirname(self.filepath)
        exported_meshes: list[str] = []
        root_nodes = []

        for obj in context.scene.objects:
            if obj.parent is None and not obj.hide_viewport and not obj.hide_get():
                root_nodes.append(_obj_to_dict(obj, root_dir, exported_meshes))

        with open(self.filepath, "w") as f:
            json.dump(root_nodes, f, indent=2)

        self.report({"INFO"}, f"Exported scene to {self.filepath}")
        return {"FINISHED"}


class RETRO_OT_import_scene(bpy.types.Operator, ImportHelper):
    """Import a Retro Engine JSON scene into the current file"""
    bl_idname = "import_scene.retro_engine"
    bl_label = "Import Retro Engine Scene"
    filename_ext = ".json"

    filter_glob: StringProperty(default="*.json", options={"HIDDEN"}, maxlen=255)

    def execute(self, context):
        with open(self.filepath, "r") as f:
            data = json.load(f)

        if not isinstance(data, list):
            self.report({"ERROR"}, "Invalid scene file: expected a JSON array at root")
            return {"CANCELLED"}

        base_dir = os.path.dirname(self.filepath)
        collection = context.collection
        mesh_cache = _MeshCache()

        for node in data:
            _import_object(node, base_dir, collection, mesh_cache, parent=None)

        # Apply FPS from first object (scene-wide in Blender)
        if data:
            context.scene.render.fps = data[0].get("animationFPS", 24)

        self.report({"INFO"}, f"Imported {len(data)} root objects from {self.filepath}")
        return {"FINISHED"}


# ─────────────────────────────────────────────────────────────────
# Side-panel
# ─────────────────────────────────────────────────────────────────

class RETRO_PT_tools(bpy.types.Panel):
    bl_label = "Retro Engine Tools"
    bl_idname = "RETRO_PT_tools"
    bl_space_type = "VIEW_3D"
    bl_region_type = "UI"
    bl_category = "Retro Engine"

    def draw(self, context):
        col = self.layout.column(align=True)
        col.operator(RETRO_OT_export_scene.bl_idname, text="Export Scene", icon="EXPORT")
        col.operator(RETRO_OT_import_scene.bl_idname, text="Import Scene", icon="IMPORT")


# ─────────────────────────────────────────────────────────────────
# Menu entries (File ▸ Import / Export)
# ─────────────────────────────────────────────────────────────────

def _menu_export(self, context):
    self.layout.operator(RETRO_OT_export_scene.bl_idname,
                         text="Retro Engine Scene (.json)")


def _menu_import(self, context):
    self.layout.operator(RETRO_OT_import_scene.bl_idname,
                         text="Retro Engine Scene (.json)")


# ─────────────────────────────────────────────────────────────────
# Registration
# ─────────────────────────────────────────────────────────────────

_classes = (
    RETRO_OT_export_scene,
    RETRO_OT_import_scene,
    RETRO_PT_tools,
)


def register():
    for cls in _classes:
        bpy.utils.register_class(cls)
    bpy.types.TOPBAR_MT_file_export.append(_menu_export)
    bpy.types.TOPBAR_MT_file_import.append(_menu_import)


def unregister():
    bpy.types.TOPBAR_MT_file_import.remove(_menu_import)
    bpy.types.TOPBAR_MT_file_export.remove(_menu_export)
    for cls in reversed(_classes):
        bpy.utils.unregister_class(cls)


if __name__ == "__main__":
    # Allow re-running from Blender's text editor
    if hasattr(bpy.types, "RETRO_OT_export_scene"):
        unregister()
    register()
