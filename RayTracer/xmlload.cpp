//-------------------------------------------------------------------------------
///
/// \file       xmlload.cpp 
/// \author     Cem Yuksel (www.cemyuksel.com)
/// \version    1.0
/// \date       August 20, 2025
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
#include "objects.h"
#include "tinyxml2.h"

//-------------------------------------------------------------------------------

void LoadScene    ( Node *rootNode, tinyxml2::XMLElement *element );
void LoadNode     ( Node *node, tinyxml2::XMLElement *element, int level );
void LoadTransform( Transformation *trans, tinyxml2::XMLElement *element, int level );
void ReadVector   ( tinyxml2::XMLElement *element, Vec3f &v );
void ReadFloat    ( tinyxml2::XMLElement *element, float  &f, char const *name="value" );

//-------------------------------------------------------------------------------

// Compares null terminated ('\0') strings.
bool StrICmp( const char *a, const char *b )
{
	if ( a == b ) return true;
	if ( !a || !b ) return false;
	int i = 0;
	while ( tolower(a[i]) == tolower(b[i]) ) {
		if ( a[i] == '\0' ) return true;
		i++;
	}
	return false;
}

//-------------------------------------------------------------------------------

int LoadScene( RenderScene &scene, char const *filename )
{
	tinyxml2::XMLDocument doc;
	tinyxml2::XMLError e = doc.LoadFile(filename);

	if ( e != tinyxml2::XML_SUCCESS ) {
		printf("Failed to load the file \"%s\"\n", filename);
		return 0;
	}

	tinyxml2::XMLElement *xml = doc.FirstChildElement("xml");
	if ( ! xml ) {
		printf("No \"xml\" tag found.\n");
		return 0;
	}

	tinyxml2::XMLElement *xscene = xml->FirstChildElement("scene");
	if ( ! xscene ) {
		printf("No \"scene\" tag found.\n");
		return 0;
	}

	tinyxml2::XMLElement *cam = xml->FirstChildElement("camera");
	if ( ! cam ) {
		printf("No \"camera\" tag found.\n");
		return 0;
	}

	scene.rootNode.Init();
	LoadScene( &scene.rootNode, xscene );

	// Load Camera
	scene.camera.Init();
	scene.camera.dir += scene.camera.pos;
	tinyxml2::XMLElement *camChild = cam->FirstChildElement();
	while ( camChild ) {
		if      ( StrICmp( camChild->Value(), "position"  ) ) ReadVector(camChild,scene.camera.pos);
		else if ( StrICmp( camChild->Value(), "target"    ) ) ReadVector(camChild,scene.camera.dir);
		else if ( StrICmp( camChild->Value(), "up"        ) ) ReadVector(camChild,scene.camera.up);
		else if ( StrICmp( camChild->Value(), "fov"       ) ) ReadFloat (camChild,scene.camera.fov);
		else if ( StrICmp( camChild->Value(), "width"     ) ) camChild->QueryIntAttribute("value", &scene.camera.imgWidth);
		else if ( StrICmp( camChild->Value(), "height"    ) ) camChild->QueryIntAttribute("value", &scene.camera.imgHeight);
		camChild = camChild->NextSiblingElement();
	}
	scene.camera.dir -= scene.camera.pos;
	scene.camera.dir.Normalize();
	Vec3f x = scene.camera.dir ^ scene.camera.up;
	scene.camera.up = (x ^ scene.camera.dir).GetNormalized();

	scene.renderImage.Init( scene.camera.imgWidth, scene.camera.imgHeight );

	return 1;
}

//-------------------------------------------------------------------------------

void PrintIndent( int level ) { for ( int i=0; i<level; i++) printf("   "); }

//-------------------------------------------------------------------------------

void LoadScene( Node *rootNode, tinyxml2::XMLElement *element )
{
	for ( tinyxml2::XMLElement *child = element->FirstChildElement(); child!=nullptr; child = child->NextSiblingElement() ) {

		if ( StrICmp( child->Value(), "object" ) ) {
			LoadNode( rootNode, child, 0 );
		}
	}
}

//-------------------------------------------------------------------------------

void LoadNode( Node *parent, tinyxml2::XMLElement *element, int level )
{
	Node *node = new Node;
	parent->AppendChild(node);

	// name
	char const *name = element->Attribute("name");
	node->SetName(name);
	PrintIndent(level);
	printf("object [");
	if ( name ) printf("%s",name);
	printf("]");

	// type
	char const *type = element->Attribute("type");
	if ( type ) {
		if ( StrICmp(type,"sphere") ) {
			node->SetNodeObj( &theSphere );
			printf(" - Sphere");
		} else {
			printf(" - UNKNOWN TYPE");
		}
	}


	printf("\n");


	for ( tinyxml2::XMLElement *child = element->FirstChildElement(); child!=nullptr; child = child->NextSiblingElement() ) {
		if ( StrICmp( child->Value(), "object" ) ) {
			LoadNode(node,child,level+1);
		}
	}
	LoadTransform( node, element, level );

}

//-------------------------------------------------------------------------------

void LoadTransform( Transformation *trans, tinyxml2::XMLElement *element, int level )
{
	for ( tinyxml2::XMLElement *child = element->FirstChildElement(); child!=nullptr; child = child->NextSiblingElement() ) {
		if ( StrICmp( child->Value(), "scale" ) ) {
			Vec3f s(1,1,1);
			ReadVector( child, s );
			trans->Scale(s.x,s.y,s.z);
			PrintIndent(level);
			printf("   scale %f %f %f\n",s.x,s.y,s.z);
		} else if ( StrICmp( child->Value(), "rotate" ) ) {
			Vec3f s(0,0,0);
			ReadVector( child, s );
			s.Normalize();
			float a;
			ReadFloat(child,a,"angle");
			trans->Rotate(s,a);
			PrintIndent(level);
			printf("   rotate %f degrees around %f %f %f\n", a, s.x, s.y, s.z);
		} else if ( StrICmp( child->Value(), "translate" ) ) {
			Vec3f t(0,0,0);
			ReadVector(child,t);
			trans->Translate(t);
			PrintIndent(level);
			printf("   translate %f %f %f\n",t.x,t.y,t.z);
		}
	}
}

//-------------------------------------------------------------------------------

void ReadVector( tinyxml2::XMLElement *element, Vec3f &v )
{
	double x = (double) v.x;
	double y = (double) v.y;
	double z = (double) v.z;
	element->QueryDoubleAttribute( "x", &x );
	element->QueryDoubleAttribute( "y", &y );
	element->QueryDoubleAttribute( "z", &z );
	v.x = (float) x;
	v.y = (float) y;
	v.z = (float) z;

	float f=1;
	ReadFloat( element, f );
	v *= f;
}

//-------------------------------------------------------------------------------

void ReadFloat ( tinyxml2::XMLElement *element, float &f, char const *name )
{
	double d = (double) f;
	element->QueryDoubleAttribute( name, &d );
	f = (float) d;
}

//-------------------------------------------------------------------------------
