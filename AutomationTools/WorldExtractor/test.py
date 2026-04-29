import unreal
import json
import os

def vector_to_dict(v): return {"x": v.x, "y": v.y, "z": v.z}
def rotator_to_dict(r): return {"pitch": r.pitch, "yaw": r.yaw, "roll": r.roll}

def extract_level_data():
    actors = unreal.EditorLevelLibrary.get_all_level_actors()
    export_data = []

    for actor in actors:
        try:
            actor_class = actor.get_class().get_name()
            actor_info = {
                "name": str(actor.get_actor_label()),
                "class": actor_class,
                "transform": {
                    "location": vector_to_dict(actor.get_actor_location()),
                    "rotation": rotator_to_dict(actor.get_actor_rotation()),
                    "scale": vector_to_dict(actor.get_actor_scale3d())
                },
                "components": []
            }

            # Handle component details
            for comp in actor.get_components_by_class(unreal.ActorComponent):
                comp_class = comp.get_class().get_name()
                comp_data = {"type": comp_class}

                if isinstance(comp, unreal.StaticMeshComponent):
                    mesh = comp.get_editor_property("static_mesh")
                    comp_data["mesh"] = mesh.get_path_name() if mesh else None
                    comp_data["materials"] = []
                    for i in range(comp.get_num_materials()):
                        mat = comp.get_material(i)
                        comp_data["materials"].append(mat.get_path_name() if mat else None)
                    comp_data["mobility"] = str(comp.mobility)

                elif isinstance(comp, unreal.SkeletalMeshComponent):
                    mesh = comp.get_editor_property("skeletal_mesh")
                    comp_data["skeletal_mesh"] = mesh.get_path_name() if mesh else None

                actor_info["components"].append(comp_data)

            # Extract spline points from WaterBody
            if "WaterBody" in actor_class:
                spline_comp = actor.get_component_by_class(unreal.WaterSplineComponent)
                if spline_comp:
                    spline_data = []
                    for i in range(spline_comp.get_number_of_spline_points()):
                        pt = spline_comp.get_location_at_spline_point(i, unreal.SplineCoordinateSpace.WORLD)
                        spline_data.append(vector_to_dict(pt))
                    actor_info["spline_points"] = spline_data

            export_data.append(actor_info)

        except Exception as e:
            unreal.log_warning(f"Skipping actor due to error: {e}")

    return export_data

def save_to_json(data, filename):
    with open(filename, "w") as f:
        json.dump(data, f, indent=4)


def main():
    output_dir = os.path.join(os.path.expanduser("~"), "Desktop")
    output_path = os.path.join(output_dir, "static_mesh_data.json")

    unreal.log("Starting full level extraction...")
    level_data = extract_level_data()
    print(level_data)
    save_to_json(level_data, output_path)
    unreal.log(f"Level data saved to {output_path}")

main()
