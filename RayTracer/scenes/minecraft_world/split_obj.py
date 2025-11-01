from collections import defaultdict
import os

def split_obj_by_material(obj_path: str):
    with open(obj_path) as f:
        lines = f.readlines()

    mtllibs = [l for l in lines if l.startswith("mtllib")]

    vertices = []
    texcoords = []
    normals = []
    faces_by_mtl = defaultdict(list)

    current_mtl = None
    for l in lines:
        if l.startswith("v "):
            vertices.append(l)
        elif l.startswith("vt "):
            texcoords.append(l)
        elif l.startswith("vn "):
            normals.append(l)
        elif l.startswith("usemtl "):
            current_mtl = l.split()[1].strip()
        elif l.startswith("f "):
            if current_mtl:
                faces_by_mtl[current_mtl].append(l.strip())

    base = os.path.splitext(obj_path)[0]

    for mtl, faces in faces_by_mtl.items():
        used_v = set()
        used_vt = set()
        used_vn = set()

        # Parse all faces and collect indices
        for face in faces:
            parts = face.split()[1:]
            for p in parts:
                idx = p.split("/")
                if idx[0]:
                    used_v.add(int(idx[0]))
                if len(idx) > 1 and idx[1]:
                    used_vt.add(int(idx[1]))
                if len(idx) > 2 and idx[2]:
                    used_vn.add(int(idx[2]))

        # Build remap tables
        v_map = {old: new for new, old in enumerate(sorted(used_v), start=1)}
        vt_map = {old: new for new, old in enumerate(sorted(used_vt), start=1)}
        vn_map = {old: new for new, old in enumerate(sorted(used_vn), start=1)}

        out_path = f"{base}_{mtl}.obj"
        with open(out_path, "w") as out:
            # Keep mtllib reference(s)
            for mtllib in mtllibs:
                out.write(mtllib)

            out.write(f"usemtl {mtl}\n")

            # Write only used vertices/texcoords/normals
            for old_idx in sorted(used_v):
                out.write(vertices[old_idx - 1])  # OBJ indices are 1-based
            for old_idx in sorted(used_vt):
                out.write(texcoords[old_idx - 1])
            for old_idx in sorted(used_vn):
                out.write(normals[old_idx - 1])

            # Rewrite faces with remapped indices
            for face in faces:
                parts = face.split()[1:]
                new_face = []
                for p in parts:
                    idx = p.split("/")
                    v = v_map[int(idx[0])] if idx[0] else ""
                    vt = vt_map[int(idx[1])] if len(idx) > 1 and idx[1] else ""
                    vn = vn_map[int(idx[2])] if len(idx) > 2 and idx[2] else ""
                    if vt or vn:
                        new_face.append(f"{v}/{vt}/{vn}".rstrip("/"))
                    else:
                        new_face.append(str(v))
                out.write("f " + " ".join(new_face) + "\n")

        print(f"Created {out_path}")

# Example usage
split_obj_by_material("minecraft_world.obj")
