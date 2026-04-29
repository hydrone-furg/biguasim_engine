import unreal
import os
import json

# Initialize editor subsystems
asset_registry = unreal.AssetRegistryHelpers.get_asset_registry()
editor_level_lib = unreal.EditorLevelLibrary

# Dictionary to store tag-name -> ID
tag_id_map = {}
next_id = 0


def get_base_tag(actor):
    """
    Determine a meaningful tag for the actor based on its mesh asset name or fallback to actor name.
    """
    # For StaticMeshActors
    if isinstance(actor, unreal.StaticMeshActor):
        static_mesh_comp = actor.static_mesh_component
        if static_mesh_comp:
            static_mesh = static_mesh_comp.static_mesh
            if static_mesh:
                return static_mesh.get_name()

    # For SkeletalMeshActors
    if isinstance(actor, unreal.SkeletalMeshActor):
        skeletal_mesh_comp = actor.skeletal_mesh_component
        if skeletal_mesh_comp:
            skeletal_mesh = skeletal_mesh_comp.skeletal_mesh
            if skeletal_mesh:
                return skeletal_mesh.get_name()

    # Fallback: use actor name prefix
    actor_name = actor.get_name()
    return actor_name.split('_')[0] if '_' in actor_name else actor_name

def process_level():
    global next_id

    # print(f"Loading level: {level_path}")
    # editor_level_lib.load_level(level_path)

    actors = editor_level_lib.get_all_level_actors()
    for actor in actors:
        actor_name = actor.get_name()
        base_tag = get_base_tag(actor)

        # Assign ID if not already done
        if base_tag not in tag_id_map:
            tag_id_map[base_tag] = next_id
            next_id += 1

        # Add the tag to the actor if not present
        tags = actor.tags
        tag_as_name = unreal.Name(base_tag)
        if tag_as_name not in tags:
            tags.append(tag_as_name)
            actor.tags = tags
            print(f"Tagged actor {actor_name} with {base_tag}")

def get_all_map_assets():
    """Return all UWorld assets (maps/levels) in the project"""
    world_class = unreal.TopLevelAssetPath("/Script/Engine.World")
    return asset_registry.get_assets_by_class(world_class, True)

process_level()

# Write palette.json
project_dir = unreal.Paths.project_dir()
json_path = os.path.join(project_dir, "palette.json")

with open(json_path, 'w') as f:
    json.dump(tag_id_map, f, indent=4)

print(f"Saved tag ID palette to {json_path}")
