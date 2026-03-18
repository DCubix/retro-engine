bl_info = {
    "name": "Retro Engine Scene Exporter",
    "author": "Diego Lopes",
    "version": (1, 0),
    "blender": (3, 0, 0),
    "location": "View3D > Tool Shelf > Export",
    "description": "Exports scene hierarchy, lights, cameras and custom properties to JSON",
    "category": "Import-Export"
}

import bpy, json, os, math, shutil
from bpy.props import StringProperty
from bpy_extras.io_utils import ExportHelper
from mathutils import Matrix, Euler

light_types = {
    "POINT": "point",
    "SUN": "directional",
    "SPOT": "spot",
    "AREA": "point" # todo: this is probably not gonna be supported by the engine...
}

GROUP_NAME = 'RE_MaterialGroup'

exported_meshes = []

def flatten(xss):
    return [x for xs in xss for x in xs]

def matrix_to_list(matrix):
    mat = [[round(f, 6) for f in row] for row in matrix]
    return flatten(mat)

def convert_blender_quat_to_opengl(quat):
    # Basis change matrix: Blender (Z-up) → OpenGL (Y-up, -Z forward)
    basis_change = Matrix((
        (1,  0,  0),
        (0,  0,  1),
        (0, -1,  0)
    ))

    # Convert quaternion to rotation matrix
    rot_mat = quat.to_matrix()

    # Apply basis change: R' = B * R * B⁻¹
    converted = basis_change @ rot_mat @ basis_change.inverted()

    # Convert back to quaternion
    return converted.to_quaternion()

def get_obj_rotation(obj):
    original_quat = obj.rotation_quaternion.copy()
    if obj.rotation_mode != 'QUATERNION':
        original_quat = obj.rotation_euler.to_quaternion()

    return original_quat

def export_to_gltf(obj, root_dir):
    models_dir = os.path.join(root_dir, "models")
    os.makedirs(models_dir, exist_ok=True)

    bpy.ops.object.select_all(action='DESELECT')

    def collect_hierarchy(obj):
        yield obj
        for child in obj.children:
            yield from collect_hierarchy(child)
    
    bpy.context.view_layer.objects.active = obj

    for o in collect_hierarchy(obj):
        o.select_set(True)
    
    gltf_path = os.path.join(models_dir, f"{obj.data.name}.glb")
    ret_path = os.path.relpath(gltf_path, root_dir).replace("\\", "/")
    
    if ret_path in exported_meshes:
        return ret_path
    
    exported_meshes.append(ret_path)

    bpy.ops.export_scene.gltf(
        filepath=gltf_path,
        export_format='GLB',
        export_import_convert_lighting_mode='COMPAT',
        use_selection=True,
        export_yup=True,
        export_colors=True,
        export_materials='NONE',
        export_cameras=False,
        export_lights=False,
        export_animations=True,
        export_extras=True,
        #export_texture_dir=".",  # Saves textures next to .gltf
        #export_image_format='AUTO',
        #export_jpeg_quality=90,
        export_apply=True
    )

    # relative path for the glTF file
    return ret_path

def find_custom_group_node(mat):
    if not mat.use_nodes:
        return None

    for node in mat.node_tree.nodes:
        if node.type == 'GROUP' and node.node_tree and node.node_tree.name == GROUP_NAME:
            return node
    return None

def extract_textures_from_node_group(group_node, root_dir):
    models_dir = os.path.join(root_dir, "models")
    os.makedirs(models_dir, exist_ok=True)
    
    texture_paths = {}
    for input_socket in group_node.inputs:
        if input_socket.is_linked:
            linked_node = input_socket.links[0].from_node
            if linked_node.type == 'TEX_IMAGE':
                abs_path = bpy.path.abspath(linked_node.image.filepath)
                tex_path = os.path.join(models_dir, os.path.basename(abs_path))
                fpath = os.path.relpath(tex_path, root_dir).replace("\\", "/")
                texture_paths[input_socket.name.lower()] = fpath
                
                # copy the texture to the path
                shutil.copy(abs_path, tex_path)
                
    return texture_paths

def export_object_material_data(obj, root_dir):
    ret = {
        "diffuseColor": [1.0, 1.0, 1.0, 1.0],
        "roughness": 1.0,
        "metallic": 0.0,
        "emissive": 0.0,
        "castsShadows": False,
        "textures": {}
    }
    
    if not obj or not obj.data.materials:
        return ret

    mat = obj.data.materials[0]
    group_node = find_custom_group_node(mat)
    
    ret["castsShadows"] = mat.shadow_method != 'NONE'
    
    if group_node:
        ret["diffuseColor"] = [*group_node.inputs['Diffuse'].default_value]
        ret["roughness"] = group_node.inputs['Roughness'].default_value
        ret["metallic"] = group_node.inputs['Metallic'].default_value
        ret["emissive"] = group_node.inputs['Emissive Strength'].default_value
        ret["textures"] = extract_textures_from_node_group(group_node, root_dir)
    
    return ret

def export_simple_object_action(obj):
    action = obj.animation_data.action if obj.animation_data else None
    if not action:
        return {}
    
    keyframes = set()
    for fcurve in action.fcurves:
        for kp in fcurve.keyframe_points:
            keyframes.add(int(kp.co.x))
            
    keyframes = sorted(list(keyframes))
    
    animation_data = []
    
    for frame in keyframes:
        bpy.context.scene.frame_set(frame)
        
        obj_loc, obj_rot, obj_scl = obj.matrix_local.decompose()
        obj_rot = convert_blender_quat_to_opengl(obj_rot)
        loc = [obj_loc.x, obj_loc.z, -obj_loc.y]
        rot = [obj_rot.x, obj_rot.y, obj_rot.z, obj_rot.w]
        scl = [obj_scl.x, obj_scl.z, obj_scl.y]
        
        animation_data.append({
            "frame": frame,
            "position": loc,
            "rotation": rot,
            "scale": scl
        })

    return None if len(animation_data) == 0 else animation_data

def node_to_dict(obj, root_dir):
    obj_loc, obj_rot, obj_scl = obj.matrix_local.decompose()
    obj_rot = convert_blender_quat_to_opengl(obj_rot)
    loc = [obj_loc.x, obj_loc.z, -obj_loc.y]
    rot = [obj_rot.x, obj_rot.y, obj_rot.z, obj_rot.w]
    scl = [obj_scl.x, obj_scl.z, obj_scl.y]
    
    node = {
        "name": obj.name,
        "type": "model",
        "position": [*loc],
        "rotation": [*rot],
        "scale": [*scl],
        "children": [],
        "animationFPS": bpy.context.scene.render.fps,
        "animation": export_simple_object_action(obj)
    }
    
    # custom properties
    for prop_name in obj.keys():
        if prop_name.startswith("_"):
            continue
        node[f"${prop_name}"] = obj[prop_name]

    if obj.type == "LIGHT":
        node["type"] = "light"
        light = obj.data
        node["color"] = [round(c, 3) for c in light.color[:]]
        node["radius"] = round(light.cutoff_distance, 4)
        node["intensity"] = light.energy / 1000.0 if light.type != 'SUN' else light.energy
        if light.type == 'SPOT':
            node["spot_angle"] = round(light.spot_size, 6)
        node["kind"] = light_types[light.type]
        node["shadows"] = light.use_shadow
    elif obj.type == "CAMERA":
        node["type"] = "camera"
        cam = obj.data
        node["fovy"] = round(cam.angle_y, 6)
        node["znear"] = cam.clip_start
        node["zfar"] = cam.clip_end
    elif obj.type == "MESH":
        node["type"] = "mesh"
        node["path"] = export_to_gltf(obj, root_dir)
        node["material"] = export_object_material_data(obj, root_dir)
    
    for child in obj.children:
        node["children"].append(node_to_dict(child, root_dir))
    
    return node

class EXPORT_SCENE_OT_export(bpy.types.Operator, ExportHelper):
    bl_idname = "export_scene.retro_engine"
    bl_label = "Export Scene"

    filename_ext = ".json"

    filter_glob: StringProperty(
        default="*.json",
        options={'HIDDEN'},
        maxlen=255,  # Max internal buffer length, longer would be clamped.
    )

    def execute(self, context):
        root_dir = os.path.dirname(self.filepath)
        root_nodes = []
        exported_meshes = []
        
        for obj in bpy.context.scene.objects:
            if obj.parent is None and not obj.hide_viewport and not obj.hide_get():
                root_nodes.append(node_to_dict(obj, root_dir))
        
        with open(self.filepath, 'w') as f:
            json.dump(root_nodes, f, indent=2)
        
        self.report({'INFO'}, f"Exported scene to {self.filepath}")

        return {'FINISHED'}

class EXPORT_SCENE_PT_retro_engine_panel(bpy.types.Panel):
    """Creates a Panel in the Object properties window"""
    bl_label = "Retro Engine Tools"
    bl_idname = "EXPORT_SCENE_PT_retro_engine_panel"
    bl_space_type = 'VIEW_3D'
    bl_region_type = 'UI'
    bl_category = "Retro Engine Tools"

    def draw(self, context):
        layout = self.layout
        layout.operator(EXPORT_SCENE_OT_export.bl_idname, text="Export Scene", icon='EXPORT')
    
classes = (
    EXPORT_SCENE_OT_export,
    EXPORT_SCENE_PT_retro_engine_panel
)

def create_custom_exporter_node():
    if GROUP_NAME in bpy.data.node_groups:
        return
    
    node_group = bpy.data.node_groups.new(GROUP_NAME, 'ShaderNodeTree')
    
    inputs = node_group.nodes.new('NodeGroupInput')
    inputs.location = (-800, 0)

    input_sockets = [
        ('NodeSocketColor', 'Diffuse'),
        ('NodeSocketColor', 'Normal'),
        ('NodeSocketFloatFactor', 'Roughness'),
        ('NodeSocketFloatFactor', 'Metallic'),
        ('NodeSocketColor', 'Emissive'),
        ('NodeSocketFloat', 'Emissive Strength'),
        ('NodeSocketFloat', 'Displacement'),
    ]
    
    for sock_type, name in input_sockets:
        node_group.inputs.new(sock_type, name)
    
    node_group.inputs.get('Diffuse').default_value = (1.0, 1.0, 1.0, 1.0)
    node_group.inputs.get('Normal').default_value = (0.5, 0.5, 1.0, 1.0)
    node_group.inputs.get('Roughness').default_value = 1.0
    node_group.inputs.get('Roughness').min_value = 0.0
    node_group.inputs.get('Roughness').max_value = 1.0
    node_group.inputs.get('Metallic').min_value = 0.0
    node_group.inputs.get('Metallic').max_value = 1.0
    
    outputs = node_group.nodes.new('NodeGroupOutput')
    outputs.location = (400, 0)
    node_group.outputs.new('NodeSocketShader', 'Shader')
    
    bsdf = node_group.nodes.new('ShaderNodeBsdfPrincipled')
    bsdf.location = (-200, 0)
    bsdf.inputs['Specular'].default_value = 0.0
    
    node_group.links.new(inputs.outputs['Diffuse'], bsdf.inputs['Base Color'])
    node_group.links.new(inputs.outputs['Roughness'], bsdf.inputs['Roughness'])
    node_group.links.new(inputs.outputs['Metallic'], bsdf.inputs['Metallic'])
    node_group.links.new(inputs.outputs['Emissive'], bsdf.inputs['Emission'])
    node_group.links.new(inputs.outputs['Emissive Strength'], bsdf.inputs['Emission Strength'])
    
    normal_map = node_group.nodes.new('ShaderNodeNormalMap')
    normal_map.location = (-500, -200)
    
    node_group.links.new(inputs.outputs['Normal'], normal_map.inputs['Color'])
    node_group.links.new(normal_map.outputs['Normal'], bsdf.inputs['Normal'])
    
    node_group.links.new(bsdf.outputs['BSDF'], outputs.inputs['Shader'])
    
    return node_group

def register():
    create_custom_exporter_node()
    for cls in classes:
        bpy.utils.register_class(cls)

def unregister():
    for cls in classes:
        bpy.utils.unregister_class(cls)

if __name__ == "__main__":
    # check if the addon is already registered
    if hasattr(bpy.types, "EXPORT_SCENE_OT_export"):
        unregister()
    register()
    # The following line is for testing the script in Blender's text editor
    # bpy.ops.export_scene.retro_engine('INVOKE_DEFAULT')