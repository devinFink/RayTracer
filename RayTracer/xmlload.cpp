
//-------------------------------------------------------------------------------
///
/// \file       xmlload.cpp 
/// \author     Cem Yuksel (www.cemyuksel.com)
/// \version    5.0
/// \date       September 13, 2025
///
/// \brief Example source for CS 6620 - University of Utah.
///
/// Copyright (c) 2019 Cem Yuksel. All Rights Reserved.
///
/// This code is provided for educational use only. Redistribution, sharing, or 
/// sublicensing of this code or its derivatives is strictly prohibited.
///
//-------------------------------------------------------------------------------

#include "scene.h"
#include "materials.h"
#include "lights.h"
#include "tinyxml2.h"
#include <objects.h>

//-------------------------------------------------------------------------------
// How to use:

// Call this function to load a scene from an xml file.
int LoadScene(RenderScene& scene, char const* filename);

//-------------------------------------------------------------------------------

Sphere theSphere;
Plane thePlane;

//-------------------------------------------------------------------------------

using namespace tinyxml2;

//-------------------------------------------------------------------------------

void LoadScene(RenderScene& scene, XMLElement* element);
void LoadNode(Node& parent, XMLElement* element, int level, ObjFileList& objList);
void LoadTransform(Transformation& trans, XMLElement* element, int level);
void LoadMaterial(MaterialList& materials, XMLElement* element);
void LoadLight(LightList& lights, XMLElement* element);
void ReadVector(XMLElement* element, Vec3f& v);
void ReadColor(XMLElement* element, Color& c);
void ReadFloat(XMLElement* element, float& f, char const* name = "value");
void SetNodeMaterials(Node* node, MaterialList const& materials);

//-------------------------------------------------------------------------------

// Compares null terminated ('\0') strings.
bool StrICmp(char const* a, char const* b)
{
    if (a == b) return true;
    if (!a || !b) return false;
    int i = 0;
    while (tolower(a[i]) == tolower(b[i])) {
        if (a[i] == '\0') return true;
        i++;
    }
    return false;
}

//-------------------------------------------------------------------------------

int LoadScene(RenderScene& scene, char const* filename)
{
    XMLDocument doc;
    XMLError e = doc.LoadFile(filename);

    if (e != XML_SUCCESS) {
        printf("Failed to load the file \"%s\"\n", filename);
        return 0;
    }

    XMLElement* xml = doc.FirstChildElement("xml");
    if (!xml) {
        printf("No \"xml\" tag found.\n");
        return 0;
    }

    XMLElement* xscene = xml->FirstChildElement("scene");
    if (!xscene) {
        printf("No \"scene\" tag found.\n");
        return 0;
    }

    XMLElement* cam = xml->FirstChildElement("camera");
    if (!cam) {
        printf("No \"camera\" tag found.\n");
        return 0;
    }

    scene.rootNode.Init();
    scene.materials.DeleteAll();
    scene.lights.DeleteAll();
    scene.objList.Clear();
    LoadScene(scene, xscene);

    scene.rootNode.ComputeChildBoundBox();

    // Assign materials
    SetNodeMaterials(&scene.rootNode, scene.materials);

    // Load Camera
    scene.camera.Init();
    scene.camera.dir += scene.camera.pos;
    XMLElement* camChild = cam->FirstChildElement();
    while (camChild) {
        if (StrICmp(camChild->Value(), "position")) ReadVector(camChild, scene.camera.pos);
        else if (StrICmp(camChild->Value(), "target")) ReadVector(camChild, scene.camera.dir);
        else if (StrICmp(camChild->Value(), "up")) ReadVector(camChild, scene.camera.up);
        else if (StrICmp(camChild->Value(), "fov")) ReadFloat(camChild, scene.camera.fov);
        else if (StrICmp(camChild->Value(), "width")) camChild->QueryIntAttribute("value", &scene.camera.imgWidth);
        else if (StrICmp(camChild->Value(), "height")) camChild->QueryIntAttribute("value", &scene.camera.imgHeight);
        camChild = camChild->NextSiblingElement();
    }
    scene.camera.dir -= scene.camera.pos;
    scene.camera.dir.Normalize();
    Vec3f x = scene.camera.dir ^ scene.camera.up;
    scene.camera.up = (x ^ scene.camera.dir).GetNormalized();

    scene.renderImage.Init(scene.camera.imgWidth, scene.camera.imgHeight);

    return 1;
}

//-------------------------------------------------------------------------------

void PrintIndent(int level) { for (int i = 0; i < level; i++) printf("   "); }

//-------------------------------------------------------------------------------

void LoadScene(RenderScene& scene, XMLElement* element)
{
    for (XMLElement* child = element->FirstChildElement(); child != nullptr; child = child->NextSiblingElement()) {
        if (StrICmp(child->Value(), "object")) {
            LoadNode(scene.rootNode, child, 0, scene.objList);
        }
        else if (StrICmp(child->Value(), "material")) {
            LoadMaterial(scene.materials, child);
        }
        else if (StrICmp(child->Value(), "light")) {
            LoadLight(scene.lights, child);
        }
    }
}

//-------------------------------------------------------------------------------

void LoadNode(Node& parent, XMLElement* element, int level, ObjFileList& objList)
{
    Node* node = new Node;
    parent.AppendChild(node);

    // name
    char const* name = element->Attribute("name");
    node->SetName(name);
    PrintIndent(level);
    printf("object [");
    if (name) printf("%s", name);
    printf("]");

    // material
    char const* mtlName = element->Attribute("material");
    if (mtlName) {
        printf(" <%s>", mtlName);
        node->SetMaterial((Material*)mtlName); // temporarily set the material pointer to a string of the material name
    }

    // type
    char const* type = element->Attribute("type");
    if (type) {
        if (StrICmp(type, "sphere")) {
            node->SetNodeObj(&theSphere);
            printf(" - Sphere");
        }
        else if (StrICmp(type, "plane")) {
            node->SetNodeObj(&thePlane);
            printf(" - Plane");
        }
        else if (StrICmp(type, "obj")) {
            printf(" - OBJ");
            Object* obj = objList.Find(name);
            if (obj == nullptr) { // object is not on the list, so we should load it now
                TriObj* tobj = new TriObj;
                if (!tobj->Load(name)) {
                    printf(" -- ERROR: Cannot load file \"%s.\"", name);
                    delete tobj;
                }
                else {
                    objList.Append(tobj, name);  // add to the list
                    obj = tobj;
                }
            }
            node->SetNodeObj(obj);
        }
        else {
            printf(" - UNKNOWN TYPE");
        }
    }


    printf("\n");


    for (XMLElement* child = element->FirstChildElement(); child != nullptr; child = child->NextSiblingElement()) {
        if (StrICmp(child->Value(), "object")) {
            LoadNode(*node, child, level + 1, objList);
        }
    }
    LoadTransform(*node, element, level);

}

//-------------------------------------------------------------------------------

void LoadTransform(Transformation& trans, XMLElement* element, int level)
{
    for (XMLElement* child = element->FirstChildElement(); child != nullptr; child = child->NextSiblingElement()) {
        if (StrICmp(child->Value(), "scale")) {
            Vec3f s(1, 1, 1);
            ReadVector(child, s);
            trans.Scale(s.x, s.y, s.z);
            PrintIndent(level);
            printf("   scale %f %f %f\n", s.x, s.y, s.z);
        }
        else if (StrICmp(child->Value(), "rotate")) {
            Vec3f s(0, 0, 0);
            ReadVector(child, s);
            s.Normalize();
            float a = 0.0f;
            ReadFloat(child, a, "angle");
            trans.Rotate(s, a);
            PrintIndent(level);
            printf("   rotate %f degrees around %f %f %f\n", a, s.x, s.y, s.z);
        }
        else if (StrICmp(child->Value(), "translate")) {
            Vec3f t(0, 0, 0);
            ReadVector(child, t);
            trans.Translate(t);
            PrintIndent(level);
            printf("   translate %f %f %f\n", t.x, t.y, t.z);
        }
    }
}

//-------------------------------------------------------------------------------

void LoadMaterial(MaterialList& materials, XMLElement* element)
{
    Material* mtl = nullptr;

    // name
    char const* name = element->Attribute("name");
    printf("Material [");
    if (name) printf("%s", name);
    printf("]");

    auto loadPhongBlinn = [&element](MtlBasePhongBlinn* m)
        {
            for (XMLElement* child = element->FirstChildElement(); child != nullptr; child = child->NextSiblingElement()) {
                Color c(1, 1, 1);
                float f = 1;
                if (StrICmp(child->Value(), "diffuse")) {
                    ReadColor(child, c);
                    m->SetDiffuse(c);
                    printf("   diffuse %f %f %f\n", c.r, c.g, c.b);
                }
                else if (StrICmp(child->Value(), "specular")) {
                    ReadColor(child, c);
                    m->SetSpecular(c);
                    printf("   specular %f %f %f\n", c.r, c.g, c.b);
                }
                else if (StrICmp(child->Value(), "glossiness")) {
                    ReadFloat(child, f);
                    m->SetGlossiness(f);
                    printf("   glossiness %f\n", f);
                }
                else if (StrICmp(child->Value(), "reflection")) {
                    ReadColor(child, c);
                    m->SetReflection(c);
                    printf("   reflection %f %f %f\n", c.r, c.g, c.b);
                }
                else if (StrICmp(child->Value(), "refraction")) {
                    ReadColor(child, c);
                    m->SetRefraction(c);
                    ReadFloat(child, f, "index");
                    m->SetIOR(f);
                    printf("   refraction %f %f %f (ior: %f)\n", c.r, c.g, c.b, f);
                }
                else if (StrICmp(child->Value(), "absorption")) {
                    ReadColor(child, c);
                    m->SetAbsorption(c);
                    printf("   absorption %f %f %f\n", c.r, c.g, c.b);
                }
            }
            return m;
        };

    // type
    char const* type = element->Attribute("type");
    if (type) {
        if (StrICmp(type, "phong")) {
            printf(" - Phong\n");
            mtl = loadPhongBlinn(new MtlPhong());
        }
        else if (StrICmp(type, "blinn")) {
            printf(" - Blinn\n");
            mtl = loadPhongBlinn(new MtlBlinn());
        }
        else if (StrICmp(type, "microfacet")) {
            printf(" - Microfacet\n");
            MtlMicrofacet* m = new MtlMicrofacet();
            mtl = m;
            for (XMLElement* child = element->FirstChildElement(); child != nullptr; child = child->NextSiblingElement()) {
                Color c(1, 1, 1);
                float f = 1;
                if (StrICmp(child->Value(), "color")) {
                    ReadColor(child, c);
                    m->SetBaseColor(c);
                    printf("   color %f %f %f\n", c.r, c.g, c.b);
                }
                else if (StrICmp(child->Value(), "roughness")) {
                    ReadFloat(child, f);
                    m->SetRoughness(f);
                    printf("   roughness %f\n", f);
                }
                else if (StrICmp(child->Value(), "metallic")) {
                    ReadFloat(child, f);
                    m->SetMetallic(f);
                    printf("   metallic %f\n", f);
                }
                else if (StrICmp(child->Value(), "ior")) {
                    ReadFloat(child, f);
                    m->SetIOR(f);
                    printf("   ior %f\n", f);
                }
                else if (StrICmp(child->Value(), "transmittance")) {
                    ReadColor(child, c);
                    m->SetTransmittance(c);
                    printf("   transmittance %f %f %f\n", c.r, c.g, c.b);
                }
                else if (StrICmp(child->Value(), "absorption")) {
                    ReadColor(child, c);
                    m->SetAbsorption(c);
                    printf("   absorption %f %f %f\n", c.r, c.g, c.b);
                }
            }
        }
        else {
            printf(" - UNKNOWN\n");
        }
    }

    if (mtl) {
        mtl->SetName(name);
        materials.push_back(mtl);
    }
}

//-------------------------------------------------------------------------------

void LoadLight(LightList& lights, XMLElement* element)
{
    Light* light = nullptr;

    // name
    char const* name = element->Attribute("name");
    printf("Light [");
    if (name) printf("%s", name);
    printf("]");

    // type
    char const* type = element->Attribute("type");
    if (type) {
        if (StrICmp(type, "ambient")) {
            printf(" - Ambient\n");
            AmbientLight* l = new AmbientLight();
            light = l;
            for (XMLElement* child = element->FirstChildElement(); child != nullptr; child = child->NextSiblingElement()) {
                if (StrICmp(child->Value(), "intensity")) {
                    Color c(1, 1, 1);
                    ReadColor(child, c);
                    l->SetIntensity(c);
                    printf("   intensity %f %f %f\n", c.r, c.g, c.b);
                }
            }
        }
        else if (StrICmp(type, "direct")) {
            printf(" - Direct\n");
            DirectLight* l = new DirectLight();
            light = l;
            for (XMLElement* child = element->FirstChildElement(); child != nullptr; child = child->NextSiblingElement()) {
                if (StrICmp(child->Value(), "intensity")) {
                    Color c(1, 1, 1);
                    ReadColor(child, c);
                    l->SetIntensity(c);
                    printf("   intensity %f %f %f\n", c.r, c.g, c.b);
                }
                else if (StrICmp(child->Value(), "direction")) {
                    Vec3f v(1, 1, 1);
                    ReadVector(child, v);
                    l->SetDirection(v);
                    printf("   direction %f %f %f\n", v.x, v.y, v.z);
                }
            }
        }
        else if (StrICmp(type, "point")) {
            printf(" - Point\n");
            PointLight* l = new PointLight();
            light = l;
            for (XMLElement* child = element->FirstChildElement(); child != nullptr; child = child->NextSiblingElement()) {
                if (StrICmp(child->Value(), "intensity")) {
                    Color c(1, 1, 1);
                    ReadColor(child, c);
                    l->SetIntensity(c);
                    printf("   intensity %f %f %f\n", c.r, c.g, c.b);
                }
                else if (StrICmp(child->Value(), "position")) {
                    Vec3f v(0, 0, 0);
                    ReadVector(child, v);
                    l->SetPosition(v);
                    printf("   position %f %f %f\n", v.x, v.y, v.z);
                }
            }
        }
        else {
            printf(" - UNKNOWN\n");
        }
    }

    if (light) {
        light->SetName(name);
        lights.push_back(light);
    }

}

//-------------------------------------------------------------------------------

void ReadVector(XMLElement* element, Vec3f& v)
{
    element->QueryFloatAttribute("x", &v.x);
    element->QueryFloatAttribute("y", &v.y);
    element->QueryFloatAttribute("z", &v.z);

    float f = 1;
    ReadFloat(element, f);
    v *= f;
}

//-------------------------------------------------------------------------------

void ReadColor(XMLElement* element, Color& c)
{
    element->QueryFloatAttribute("r", &c.r);
    element->QueryFloatAttribute("g", &c.g);
    element->QueryFloatAttribute("b", &c.b);

    float f = 1;
    ReadFloat(element, f);
    c *= f;
}

//-------------------------------------------------------------------------------

void ReadFloat(XMLElement* element, float& f, char const* name)
{
    element->QueryFloatAttribute(name, &f);
}

//-------------------------------------------------------------------------------

void SetNodeMaterials(Node* node, MaterialList const& materials)
{
    int n = node->GetNumChild();
    if (node->GetMaterial()) {
        const char* mtlName = (const char*)node->GetMaterial();
        Material* mtl = materials.Find(mtlName);  // mtl can be null
        node->SetMaterial(mtl);
    }
    for (int i = 0; i < n; i++) SetNodeMaterials(node->GetChild(i), materials);
}

//-------------------------------------------------------------------------------
