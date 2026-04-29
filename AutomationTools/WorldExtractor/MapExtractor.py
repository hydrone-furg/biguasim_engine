import unreal
import json
import os

def get_asset_path(asset):
    if asset:
        return asset.get_path_name()
    return ""

def get_material_path(material):
    if material:
        return material.get_path_name()
    return ""

def main():
    all_actors = unreal.EditorLevelLibrary.get_all_level_actors()
    static_mesh_data = []

    print(all_actors)

    for actor in all_actors:
        if isinstance(actor, unreal.StaticMeshActor):
            name = actor.get_actor_label()

            location = actor.get_actor_location()
            scale = actor.get_actor_scale3d()
            rotation = actor.get_actor_rotation()

            static_mesh_comp = actor.static_mesh_component
            static_mesh = static_mesh_comp.static_mesh if static_mesh_comp else None
            mesh_path = get_asset_path(static_mesh)

            material_path = ""
            if static_mesh_comp and static_mesh_comp.get_num_materials() > 0:
                mat = static_mesh_comp.get_material(0)
                material_path = get_material_path(mat)

            static_mesh_data.append({
                "Name": name,
                "posicao": {
                    "X": location.x,
                    "Y": location.y,
                    "Z": location.z
                },
                "escala": {
                    "X": scale.x,
                    "Y": scale.y,
                    "Z": scale.z
                },
                "Rotacao": {
                    "Pitch": rotation.pitch,
                    "Yaw": rotation.yaw,
                    "Roll": rotation.roll
                },
                "Mesh": mesh_path,
                "Material": material_path
            })

    json_data = {
        "StaticMeshs": static_mesh_data
    }

    desktop_path = os.path.join(os.path.expanduser("~"), "Desktop")
    save_path = os.path.join(desktop_path, "static_mesh_data.json")

    with open(save_path, 'w') as f:
        json.dump(json_data, f, indent=4)

    print(f"JSON salvo em: {save_path}")

if __name__ == "__main__":
    main()
