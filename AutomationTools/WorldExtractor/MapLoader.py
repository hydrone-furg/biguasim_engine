import unreal
import json
import os

def load_asset(asset_path):
    """Carrega um asset pelo caminho."""
    if not asset_path:
        return None
    return unreal.EditorAssetLibrary.load_asset(asset_path)

def main():
    desktop_path = os.path.join(os.path.expanduser("~"), "Desktop")
    json_path = os.path.join(desktop_path, "static_mesh_data.json")

    if not os.path.exists(json_path):
        print(f"Arquivo JSON não encontrado em: {json_path}")
        return

    with open(json_path, 'r') as f:
        data = json.load(f)

    for sm_data in data.get("StaticMeshs", []):
        mesh_asset = load_asset(sm_data.get("Mesh", ""))
        if not mesh_asset:
            print(f"Mesh não encontrada: {sm_data.get('Mesh')}")
            continue

        spawn_location = unreal.Vector(
            sm_data["posicao"]["X"],
            sm_data["posicao"]["Y"],
            sm_data["posicao"]["Z"]
        )
        spawn_rotation = unreal.Rotator(
            sm_data["Rotacao"]["Pitch"],
            sm_data["Rotacao"]["Roll"],
            sm_data["Rotacao"]["Yaw"]
        )

        new_actor = unreal.EditorLevelLibrary.spawn_actor_from_class(unreal.StaticMeshActor, spawn_location, spawn_rotation)

        new_actor.set_actor_scale3d(unreal.Vector(
            sm_data["escala"]["X"],
            sm_data["escala"]["Y"],
            sm_data["escala"]["Z"]
        ))

        static_mesh_comp = new_actor.static_mesh_component
        if static_mesh_comp:
            static_mesh_comp.set_static_mesh(mesh_asset)

            material_path = sm_data.get("Material", "")
            material_asset = load_asset(material_path)
            if material_asset:
                static_mesh_comp.set_material(0, material_asset)

        if sm_data.get("Name"):
            new_actor.set_actor_label(sm_data["Name"])

        print(f"Ator StaticMesh '{sm_data.get('Name')}' criado com mesh '{mesh_asset.get_name()}'")

if __name__ == "__main__":
    main()
