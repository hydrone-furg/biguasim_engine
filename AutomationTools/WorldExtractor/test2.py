import unreal
import json
import os

def dict_to_vector(d): return unreal.Vector(d["x"], d["y"], d["z"])
def dict_to_rotator(d): return unreal.Rotator(d["pitch"], d["yaw"], d["roll"])

def safe_load_class(name):
    if name.startswith("WaterBody"):
        return unreal.load_class(None, f"/Script/Water.{name}")
    known = {
        "StaticMeshActor": unreal.StaticMeshActor,
        "DirectionalLight": unreal.DirectionalLight,
        "SkyLight": unreal.SkyLight,
        "ExponentialHeightFog": unreal.ExponentialHeightFog,
        "VolumetricCloud": unreal.VolumetricCloud,
        "PlayerStart": unreal.PlayerStart,
        "SkyAtmosphere": unreal.SkyAtmosphere,
        "Landscape": unreal.Landscape
    }
    return known.get(name, None)

def apply_component_settings(actor, comp_data):
    for comp in actor.get_components_by_class(unreal.ActorComponent):
        if comp.get_class().get_name() == comp_data["type"]:
            if comp_data["type"] == "StaticMeshComponent":
                if comp_data.get("mesh"):
                    mesh = unreal.load_asset(comp_data["mesh"])
                    if mesh:
                        comp.set_editor_property("static_mesh", mesh)

                for i, mat_path in enumerate(comp_data.get("materials", [])):
                    if mat_path:
                        mat = unreal.load_asset(mat_path)
                        if mat:
                            comp.set_material(i, mat)

                if "mobility" in comp_data:
                    mob = comp_data["mobility"]
                    if "STATIC" in mob:
                        comp.set_editor_property("mobility", unreal.ComponentMobility.STATIC)
                    elif "MOVABLE" in mob:
                        comp.set_editor_property("mobility", unreal.ComponentMobility.MOVABLE)
                    elif "STATIONARY" in mob:
                        comp.set_editor_property("mobility", unreal.ComponentMobility.STATIONARY)

def spawn_actor_from_config(data):
    cls = safe_load_class(data["class"])
    if not cls:
        unreal.log_warning(f"Skipping unknown actor class: {data['class']}")
        return

    loc = dict_to_vector(data["transform"]["location"])
    rot = dict_to_rotator(data["transform"]["rotation"])
    scl = dict_to_vector(data["transform"]["scale"])

    actor = unreal.EditorLevelLibrary.spawn_actor_from_class(cls, loc, rot)
    if not actor:
        unreal.log_warning(f"Could not spawn actor: {data['name']}")
        return

    actor.set_actor_label(data["name"])
    actor.set_actor_scale3d(scl)

    for comp_data in data.get("components", []):
        apply_component_settings(actor, comp_data)

    # Rebuild WaterBody spline
    if "WaterBody" in data["class"] and "spline_points" in data:
        spline_comp = actor.get_component_by_class(unreal.WaterSplineComponent)
        if spline_comp:
            spline_comp.clear_spline_points()
            for i, pt in enumerate(data["spline_points"]):
                spline_comp.add_spline_point_at_index(dict_to_vector(pt), i, unreal.SplineCoordinateSpace.WORLD, update_spline=False)
            spline_comp.update_spline()
        else:
            unreal.log_warning(f"Water actor missing spline component: {data['name']}")

def import_level_from_json(json_path):
    with open(json_path, "r") as f:
        data = json.load(f)
    for actor_data in data:
        spawn_actor_from_config(actor_data)

def create_new_level(level_name="RebuiltLevel", save_path="/Game/GeneratedLevels"):
    if not unreal.EditorAssetLibrary.does_directory_exist(save_path):
        unreal.EditorAssetLibrary.make_directory(save_path)

    full_path = f"{save_path}/{level_name}"
    if unreal.EditorAssetLibrary.does_asset_exist(full_path):
        unreal.EditorLevelLibrary.load_level(full_path)
        return full_path

    world = unreal.EditorLevelLibrary.new_level(full_path)
    if world and unreal.EditorAssetLibrary.save_asset(full_path):
        unreal.EditorLevelLibrary.load_level(full_path)
        return full_path
    unreal.log_error("Failed to create and load level.")
    return None

def main():
    # Step 1: Create & open level
    create_new_level()

    # Step 2: Load and place actors
    output_dir = os.path.join(os.path.expanduser("~"), "Desktop")
    json_path =  os.path.join(output_dir, "static_mesh_data.json")
    import_level_from_json(json_path)

main()