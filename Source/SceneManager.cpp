///////////////////////////////////////////////////////////////////////////////
// scenemanager.cpp
// ============
// manage the preparing and rendering of 3D scenes - textures, materials, lighting
//
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
//	Created for CS-330-Computational Graphics and Visualization, Nov. 1st, 2023
///////////////////////////////////////////////////////////////////////////////

#include "SceneManager.h"

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif

#include <glm/gtx/transform.hpp>

// declaration of global variables
namespace
{
	const char* g_ModelName = "model";
	const char* g_ColorValueName = "objectColor";
	const char* g_TextureValueName = "objectTexture";
	const char* g_UseTextureName = "bUseTexture";
	const char* g_UseLightingName = "bUseLighting";

	// Photon torpedo positions.
	// Two groups of two, placed in firing paths between the ships.
	const glm::vec3 TORPEDO_A1 = glm::vec3(-0.65f, 1.05f, 1.40f);
	const glm::vec3 TORPEDO_A2 = glm::vec3(-1.00f, 0.94f, 1.64f);
	const glm::vec3 TORPEDO_B1 = glm::vec3(-0.48f, 0.42f, -0.56f);
	const glm::vec3 TORPEDO_B2 = glm::vec3(0.24f, 0.67f, -0.84f);
}

/***********************************************************
 *  SceneManager()
 *
 *  The constructor for the class
 ***********************************************************/
SceneManager::SceneManager(ShaderManager *pShaderManager)
{
	m_pShaderManager = pShaderManager;
	m_basicMeshes = new ShapeMeshes();
	m_loadedTextures = 0;
	m_parentPosition = glm::vec3(0.0f, 0.0f, 0.0f);
	m_parentRotationDegrees = glm::vec3(0.0f, 0.0f, 0.0f);
	m_parentScale = 1.0f;
}

/***********************************************************
 *  ~SceneManager()
 *
 *  The destructor for the class
 ***********************************************************/
SceneManager::~SceneManager()
{
	m_pShaderManager = NULL;
	delete m_basicMeshes;
	m_basicMeshes = NULL;
}

/***********************************************************
 *  CreateGLTexture()
 *
 *  This method is used for loading textures from image files,
 *  configuring the texture mapping parameters in OpenGL,
 *  generating the mipmaps, and loading the read texture into
 *  the next available texture slot in memory.
 ***********************************************************/
bool SceneManager::CreateGLTexture(const char* filename, std::string tag)
{
	int width = 0;
	int height = 0;
	int colorChannels = 0;
	GLuint textureID = 0;

	// indicate to always flip images vertically when loaded
	stbi_set_flip_vertically_on_load(true);

	// try to parse the image data from the specified image file
	unsigned char* image = stbi_load(
		filename,
		&width,
		&height,
		&colorChannels,
		0);

	// if the image was successfully read from the image file
	if (image)
	{
		std::cout << "Successfully loaded image:" << filename << ", width:" << width << ", height:" << height << ", channels:" << colorChannels << std::endl;

		glGenTextures(1, &textureID);
		glBindTexture(GL_TEXTURE_2D, textureID);

		// set the texture wrapping parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		// set texture filtering parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// if the loaded image is in RGB format
		if (colorChannels == 3)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
		// if the loaded image is in RGBA format - it supports transparency
		else if (colorChannels == 4)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
		else
		{
			std::cout << "Not implemented to handle image with " << colorChannels << " channels" << std::endl;
			return false;
		}

		// generate the texture mipmaps for mapping textures to lower resolutions
		glGenerateMipmap(GL_TEXTURE_2D);

		// free the image data from local memory
		stbi_image_free(image);
		glBindTexture(GL_TEXTURE_2D, 0); // Unbind the texture

		// register the loaded texture and associate it with the special tag string
		m_textureIDs[m_loadedTextures].ID = textureID;
		m_textureIDs[m_loadedTextures].tag = tag;
		m_loadedTextures++;

		return true;
	}

	std::cout << "Could not load image:" << filename << std::endl;

	// Error loading the image
	return false;
}

/***********************************************************
 *  BindGLTextures()
 *
 *  This method is used for binding the loaded textures to
 *  OpenGL texture memory slots.  There are up to 16 slots.
 ***********************************************************/
void SceneManager::BindGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		// bind textures on corresponding texture units
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  DestroyGLTextures()
 *
 *  This method is used for freeing the memory in all the
 *  used texture memory slots.
 ***********************************************************/
void SceneManager::DestroyGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		glDeleteTextures(1, &m_textureIDs[i].ID);
		m_textureIDs[i].ID = 0;
	}
	m_loadedTextures = 0;
	m_parentPosition = glm::vec3(0.0f, 0.0f, 0.0f);
	m_parentRotationDegrees = glm::vec3(0.0f, 0.0f, 0.0f);
	m_parentScale = 1.0f;
}

/***********************************************************
 *  FindTextureID()
 *
 *  This method is used for getting an ID for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureID(std::string tag)
{
	int textureID = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureID = m_textureIDs[index].ID;
			bFound = true;
		}
		else
			index++;
	}

	return(textureID);
}

/***********************************************************
 *  FindTextureSlot()
 *
 *  This method is used for getting a slot index for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureSlot(std::string tag)
{
	int textureSlot = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureSlot = index;
			bFound = true;
		}
		else
			index++;
	}

	return(textureSlot);
}

/***********************************************************
 *  FindMaterial()
 *
 *  This method is used for getting a material from the previously
 *  defined materials list that is associated with the passed in tag.
 ***********************************************************/
bool SceneManager::FindMaterial(std::string tag, OBJECT_MATERIAL& material)
{
	if (m_objectMaterials.size() == 0)
	{
		return(false);
	}

	int index = 0;
	bool bFound = false;
	while ((index < m_objectMaterials.size()) && (bFound == false))
	{
		if (m_objectMaterials[index].tag.compare(tag) == 0)
		{
			bFound = true;
			material.ambientColor = m_objectMaterials[index].ambientColor;
			material.ambientStrength = m_objectMaterials[index].ambientStrength;
			material.diffuseColor = m_objectMaterials[index].diffuseColor;
			material.specularColor = m_objectMaterials[index].specularColor;
			material.shininess = m_objectMaterials[index].shininess;
		}
		else
		{
			index++;
		}
	}

	return(true);
}

/***********************************************************
 *  SetTransformations()
 *
 *  This method is used for setting the transform buffer
 *  using the passed in transformation values.
 ***********************************************************/
void SceneManager::SetTransformations(
	glm::vec3 scaleXYZ,
	float XrotationDegrees,
	float YrotationDegrees,
	float ZrotationDegrees,
	glm::vec3 positionXYZ)
{
	// variables for this method
	glm::mat4 modelView;
	glm::mat4 scale;
	glm::mat4 rotationX;
	glm::mat4 rotationY;
	glm::mat4 rotationZ;
	glm::mat4 translation;

	// set the scale value in the transform buffer
	scale = glm::scale(scaleXYZ);
	// set the rotation values in the transform buffer
	rotationX = glm::rotate(glm::radians(XrotationDegrees), glm::vec3(1.0f, 0.0f, 0.0f));
	rotationY = glm::rotate(glm::radians(YrotationDegrees), glm::vec3(0.0f, 1.0f, 0.0f));
	rotationZ = glm::rotate(glm::radians(ZrotationDegrees), glm::vec3(0.0f, 0.0f, 1.0f));
	// set the translation value in the transform buffer
	translation = glm::translate(positionXYZ);

	modelView = translation * rotationZ * rotationY * rotationX * scale;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setMat4Value(g_ModelName, modelView);
	}
}

/***********************************************************
 *  SetShaderColor()
 *
 *  This method is used for setting the passed in color
 *  into the shader for the next draw command
 ***********************************************************/
void SceneManager::SetShaderColor(
	float redColorValue,
	float greenColorValue,
	float blueColorValue,
	float alphaValue)
{
	// variables for this method
	glm::vec4 currentColor;

	currentColor.r = redColorValue;
	currentColor.g = greenColorValue;
	currentColor.b = blueColorValue;
	currentColor.a = alphaValue;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, false);
		m_pShaderManager->setVec4Value(g_ColorValueName, currentColor);
	}
}

/***********************************************************
 *  SetShaderTexture()
 *
 *  This method is used for setting the texture data
 *  associated with the passed in ID into the shader.
 ***********************************************************/
void SceneManager::SetShaderTexture(
	std::string textureTag)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, true);

		int textureID = -1;
		textureID = FindTextureSlot(textureTag);
		m_pShaderManager->setSampler2DValue(g_TextureValueName, textureID);
	}
}

/***********************************************************
 *  SetTextureUVScale()
 *
 *  This method is used for setting the texture UV scale
 *  values into the shader.
 ***********************************************************/
void SceneManager::SetTextureUVScale(float u, float v)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setVec2Value("UVscale", glm::vec2(u, v));
	}
}

/***********************************************************
 *  SetShaderLighting()
 *
 *  Enables or disables Phong lighting for the next objects.
 *  The starfield is intentionally unlit, while the base plane
 *  and ship use the lighting setup below.
 ***********************************************************/
void SceneManager::SetShaderLighting(bool bUseLighting)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseLightingName, bUseLighting);
	}
}

/***********************************************************
 *  DefineObjectMaterials()
 *
 *  Material values control the Phong ambient, diffuse, and
 *  specular response for metal hulls, glowing pieces, and the
 *  planet layer.
 ***********************************************************/
void SceneManager::DefineObjectMaterials()
{
	OBJECT_MATERIAL material;

	material.ambientColor = glm::vec3(0.15f, 0.17f, 0.20f);
	material.ambientStrength = 0.10f;
	material.diffuseColor = glm::vec3(0.70f, 0.72f, 0.76f);
	material.specularColor = glm::vec3(0.90f, 0.92f, 0.95f);
	material.shininess = 48.0f;
	material.tag = "federationHull";
	m_objectMaterials.push_back(material);

	material.ambientColor = glm::vec3(0.08f, 0.09f, 0.11f);
	material.ambientStrength = 0.06f;
	material.diffuseColor = glm::vec3(0.38f, 0.40f, 0.45f);
	material.specularColor = glm::vec3(0.75f, 0.78f, 0.82f);
	material.shininess = 64.0f;
	material.tag = "darkMetal";
	m_objectMaterials.push_back(material);

	material.ambientColor = glm::vec3(0.06f, 0.13f, 0.10f);
	material.ambientStrength = 0.12f;
	material.diffuseColor = glm::vec3(0.40f, 0.52f, 0.38f);
	material.specularColor = glm::vec3(0.70f, 0.86f, 0.62f);
	material.shininess = 56.0f;
	material.tag = "klingonHull";
	m_objectMaterials.push_back(material);

	material.ambientColor = glm::vec3(0.03f, 0.04f, 0.035f);
	material.ambientStrength = 0.07f;
	material.diffuseColor = glm::vec3(0.20f, 0.25f, 0.22f);
	material.specularColor = glm::vec3(0.55f, 0.68f, 0.55f);
	material.shininess = 72.0f;
	material.tag = "klingonDark";
	m_objectMaterials.push_back(material);

	material.ambientColor = glm::vec3(0.08f, 0.12f, 0.20f);
	material.ambientStrength = 0.20f;
	material.diffuseColor = glm::vec3(0.45f, 0.68f, 1.00f);
	material.specularColor = glm::vec3(0.95f, 0.98f, 1.00f);
	material.shininess = 96.0f;
	material.tag = "blueGlow";
	m_objectMaterials.push_back(material);

	material.ambientColor = glm::vec3(0.30f, 0.10f, 0.02f);
	material.ambientStrength = 0.35f;
	material.diffuseColor = glm::vec3(1.00f, 0.45f, 0.12f);
	material.specularColor = glm::vec3(1.00f, 0.85f, 0.50f);
	material.shininess = 128.0f;
	material.tag = "orangeGlow";
	m_objectMaterials.push_back(material);

	material.ambientColor = glm::vec3(0.02f, 0.20f, 0.03f);
	material.ambientStrength = 0.35f;
	material.diffuseColor = glm::vec3(0.30f, 1.00f, 0.18f);
	material.specularColor = glm::vec3(0.70f, 1.00f, 0.55f);
	material.shininess = 128.0f;
	material.tag = "greenGlow";
	m_objectMaterials.push_back(material);

	material.ambientColor = glm::vec3(0.35f, 0.08f, 0.02f);
	material.ambientStrength = 0.45f;
	material.diffuseColor = glm::vec3(1.00f, 0.36f, 0.08f);
	material.specularColor = glm::vec3(1.00f, 0.80f, 0.35f);
	material.shininess = 128.0f;
	material.tag = "photonGlow";
	m_objectMaterials.push_back(material);

	material.ambientColor = glm::vec3(0.06f, 0.08f, 0.11f);
	material.ambientStrength = 0.18f;
	material.diffuseColor = glm::vec3(0.62f, 0.72f, 0.80f);
	material.specularColor = glm::vec3(0.35f, 0.42f, 0.50f);
	material.shininess = 22.0f;
	material.tag = "planetMaterial";
	m_objectMaterials.push_back(material);
}

/***********************************************************
 *  ConfigurePhotonPointLight()
 *
 *  Adds a focused red-orange point light at the
 *  projectile location so each torpedo behaves like its own
 *  small glowing light source instead of a flat red object.
 ***********************************************************/
void SceneManager::ConfigurePhotonPointLight(int lightIndex, glm::vec3 position)
{
	if (NULL == m_pShaderManager)
	{
		return;
	}

	std::string lightName = "pointLights[" + std::to_string(lightIndex) + "]";

	m_pShaderManager->setIntValue((lightName + ".bActive").c_str(), true);
	m_pShaderManager->setVec3Value((lightName + ".position").c_str(), position);
	m_pShaderManager->setVec3Value((lightName + ".ambient").c_str(), glm::vec3(0.10f, 0.025f, 0.004f));
	m_pShaderManager->setVec3Value((lightName + ".diffuse").c_str(), glm::vec3(2.20f, 0.72f, 0.08f));
	m_pShaderManager->setVec3Value((lightName + ".specular").c_str(), glm::vec3(2.60f, 1.35f, 0.32f));
	m_pShaderManager->setFloatValue((lightName + ".constant").c_str(), 1.0f);
	m_pShaderManager->setFloatValue((lightName + ".linear").c_str(), 1.40f);
	m_pShaderManager->setFloatValue((lightName + ".quadratic").c_str(), 2.80f);
}

/***********************************************************
 *  SetupSceneLights()
 *
 *  Places a yellow(sun) directional light, a blue planet-fill
 *  point light, ship accent lights, and one localized point
 *  light per photon torpedo.
 ***********************************************************/
void SceneManager::SetupSceneLights()
{
	if (NULL == m_pShaderManager)
	{
		return;
	}

	SetShaderLighting(true);

	// Warm off-screen sun from the upper-left/front. This is the primary Phong light
	// and also produces the sunlit rim on the large planet sphere.
	m_pShaderManager->setIntValue("directionalLight.bActive", true);
	m_pShaderManager->setVec3Value("directionalLight.direction", glm::vec3(-0.55f, -0.70f, -0.25f));
	m_pShaderManager->setVec3Value("directionalLight.ambient", glm::vec3(0.050f, 0.052f, 0.058f));
	m_pShaderManager->setVec3Value("directionalLight.diffuse", glm::vec3(0.88f, 0.80f, 0.66f));
	m_pShaderManager->setVec3Value("directionalLight.specular", glm::vec3(1.00f, 0.94f, 0.82f));

	// Soft blue-white fill rising from the planet surface below the battle.
	m_pShaderManager->setIntValue("pointLights[0].bActive", true);
	m_pShaderManager->setVec3Value("pointLights[0].position", glm::vec3(-1.80f, -2.10f, 3.10f));
	m_pShaderManager->setVec3Value("pointLights[0].ambient", glm::vec3(0.040f, 0.060f, 0.080f));
	m_pShaderManager->setVec3Value("pointLights[0].diffuse", glm::vec3(0.55f, 0.70f, 0.90f));
	m_pShaderManager->setVec3Value("pointLights[0].specular", glm::vec3(0.65f, 0.82f, 1.00f));
	m_pShaderManager->setFloatValue("pointLights[0].constant", 1.0f);
	m_pShaderManager->setFloatValue("pointLights[0].linear", 0.030f);
	m_pShaderManager->setFloatValue("pointLights[0].quadratic", 0.004f);

	// Orange engine accent near the upper Federation cruiser.
	m_pShaderManager->setIntValue("pointLights[1].bActive", true);
	m_pShaderManager->setVec3Value("pointLights[1].position", glm::vec3(0.80f, 1.55f, -0.25f));
	m_pShaderManager->setVec3Value("pointLights[1].ambient", glm::vec3(0.035f, 0.014f, 0.003f));
	m_pShaderManager->setVec3Value("pointLights[1].diffuse", glm::vec3(1.20f, 0.42f, 0.06f));
	m_pShaderManager->setVec3Value("pointLights[1].specular", glm::vec3(1.00f, 0.58f, 0.20f));
	m_pShaderManager->setFloatValue("pointLights[1].constant", 1.0f);
	m_pShaderManager->setFloatValue("pointLights[1].linear", 0.28f);
	m_pShaderManager->setFloatValue("pointLights[1].quadratic", 0.20f);

	// Orange engine accent near the lower-left Federation cruiser.
	m_pShaderManager->setIntValue("pointLights[2].bActive", true);
	m_pShaderManager->setVec3Value("pointLights[2].position", glm::vec3(-4.70f, -0.45f, 1.20f));
	m_pShaderManager->setVec3Value("pointLights[2].ambient", glm::vec3(0.035f, 0.014f, 0.003f));
	m_pShaderManager->setVec3Value("pointLights[2].diffuse", glm::vec3(1.15f, 0.40f, 0.06f));
	m_pShaderManager->setVec3Value("pointLights[2].specular", glm::vec3(1.00f, 0.58f, 0.20f));
	m_pShaderManager->setFloatValue("pointLights[2].constant", 1.0f);
	m_pShaderManager->setFloatValue("pointLights[2].linear", 0.28f);
	m_pShaderManager->setFloatValue("pointLights[2].quadratic", 0.20f);

	// Green Klingon accent on the foreground D7.
	m_pShaderManager->setIntValue("pointLights[3].bActive", true);
	m_pShaderManager->setVec3Value("pointLights[3].position", glm::vec3(-2.00f, 0.18f, 2.60f));
	m_pShaderManager->setVec3Value("pointLights[3].ambient", glm::vec3(0.002f, 0.025f, 0.004f));
	m_pShaderManager->setVec3Value("pointLights[3].diffuse", glm::vec3(0.22f, 1.05f, 0.16f));
	m_pShaderManager->setVec3Value("pointLights[3].specular", glm::vec3(0.58f, 1.00f, 0.42f));
	m_pShaderManager->setFloatValue("pointLights[3].constant", 1.0f);
	m_pShaderManager->setFloatValue("pointLights[3].linear", 0.28f);
	m_pShaderManager->setFloatValue("pointLights[3].quadratic", 0.20f);

	// Four photon torpedoes, each with its own attenuated point light.
	ConfigurePhotonPointLight(4, TORPEDO_A1);
	ConfigurePhotonPointLight(5, TORPEDO_A2);
	ConfigurePhotonPointLight(6, TORPEDO_B1);
	ConfigurePhotonPointLight(7, TORPEDO_B2);

	m_pShaderManager->setIntValue("spotLight.bActive", false);
}

/***********************************************************
 *  SetShaderMaterial()
 *
 *  This method is used for passing the material values
 *  into the shader.
 ***********************************************************/
void SceneManager::SetShaderMaterial(
	std::string materialTag)
{
	if (m_objectMaterials.size() > 0)
	{
		OBJECT_MATERIAL material;
		bool bReturn = false;

		bReturn = FindMaterial(materialTag, material);
		if (bReturn == true)
		{
			m_pShaderManager->setVec3Value("material.ambientColor", material.ambientColor);
			m_pShaderManager->setFloatValue("material.ambientStrength", material.ambientStrength);
			m_pShaderManager->setVec3Value("material.diffuseColor", material.diffuseColor);
			m_pShaderManager->setVec3Value("material.specularColor", material.specularColor);
			m_pShaderManager->setFloatValue("material.shininess", material.shininess);
		}
	}
}

/**************************************************************/
/*** STUDENTS CAN MODIFY the code in the methods BELOW for  ***/
/*** preparing and rendering their own 3D replicated scenes.***/
/*** Please refer to the code in the OpenGL sample project  ***/
/*** for assistance.                                        ***/
/**************************************************************/

/***********************************************************
 *  SetShipParentTransform()
 *
 *  Stores the placement for the next multi-part ship. Each
 *  primitive can then be modeled in local space and inherited
 *  into the ship's final world position, rotation, and scale.
 ***********************************************************/
void SceneManager::SetShipParentTransform(
	glm::vec3 parentPosition,
	glm::vec3 parentRotationDegrees,
	float parentScale)
{
	m_parentPosition = parentPosition;
	m_parentRotationDegrees = parentRotationDegrees;
	m_parentScale = parentScale;
}

/***********************************************************
 *  SetShipPartTransform()
 *
 *  Applies the active parent transform to a single primitive.
 *  This keeps the Federation and Klingon ships coherent when
 *  each whole ship is copied to a new X, Y, Z coordinate.
 ***********************************************************/
void SceneManager::SetShipPartTransform(
	glm::vec3 localScale,
	glm::vec3 localRotationDegrees,
	glm::vec3 localPosition)
{
	glm::mat4 localTransform = glm::translate(localPosition)
		* glm::rotate(glm::radians(localRotationDegrees.z), glm::vec3(0.0f, 0.0f, 1.0f))
		* glm::rotate(glm::radians(localRotationDegrees.y), glm::vec3(0.0f, 1.0f, 0.0f))
		* glm::rotate(glm::radians(localRotationDegrees.x), glm::vec3(1.0f, 0.0f, 0.0f))
		* glm::scale(localScale);

	glm::mat4 parentTransform = glm::translate(m_parentPosition)
		* glm::rotate(glm::radians(m_parentRotationDegrees.z), glm::vec3(0.0f, 0.0f, 1.0f))
		* glm::rotate(glm::radians(m_parentRotationDegrees.y), glm::vec3(0.0f, 1.0f, 0.0f))
		* glm::rotate(glm::radians(m_parentRotationDegrees.x), glm::vec3(1.0f, 0.0f, 0.0f))
		* glm::scale(glm::vec3(m_parentScale, m_parentScale, m_parentScale));

	glm::mat4 modelView = parentTransform * localTransform;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setMat4Value(g_ModelName, modelView);
	}
}

/***********************************************************
 *  SetCruiserPartTransform()
 *
 *  Wrapper retained so the original Constitution-class helper
 *  functions did not need to be rewritten one by one.
 ***********************************************************/
void SceneManager::SetCruiserPartTransform(
	glm::vec3 localScale,
	glm::vec3 localRotationDegrees,
	glm::vec3 localPosition)
{
	SetShipPartTransform(localScale, localRotationDegrees, localPosition);
}

/***********************************************************
 *  PrepareScene()
 *
 *  Loads one copy of each reusable primitive mesh and the
 *  procedural textures used by the final scene.
 ***********************************************************/
void SceneManager::PrepareScene()
{
	// Only one instance of a particular mesh needs to be loaded in memory.
	m_basicMeshes->LoadPlaneMesh();
	m_basicMeshes->LoadBoxMesh();
	m_basicMeshes->LoadCylinderMesh();
	m_basicMeshes->LoadSphereMesh();
	m_basicMeshes->LoadConeMesh();
	m_basicMeshes->LoadTorusMesh();
	// All textures generated with OpenAI
	// Federation cruiser textures
	CreateGLTexture("textures/ship_hull_panels.jpg", "shipHull");
	CreateGLTexture("textures/dark_hull_panels.jpg", "darkHull");
	CreateGLTexture("textures/blue_deflector.jpg", "blueDeflector");
	CreateGLTexture("textures/orange_engine_glow.jpg", "engineGlow");
	CreateGLTexture("textures/starfield.jpg", "starfield");
	CreateGLTexture("textures/base_grid.jpg", "baseGrid");

	// Klingon textures
	CreateGLTexture("textures/klingon_hull_panels.jpg", "klingonHull");
	CreateGLTexture("textures/klingon_dark_panels.jpg", "klingonDark");
	CreateGLTexture("textures/green_engine_glow.jpg", "greenGlow");
	CreateGLTexture("textures/photon_torpedo_glow.jpg", "photonGlow");
	CreateGLTexture("textures/planet_clouds.jpg", "planetClouds");

	BindGLTextures();

	DefineObjectMaterials();
	SetupSceneLights();
}

/***********************************************************
 *  DrawStarfieldBackdrop()
 *
 *  A vertical background plane so the scene reads as space
 *  behind the ship battle instead of as an empty black window.
 ***********************************************************/
void SceneManager::DrawStarfieldBackdrop()
{
	SetTransformations(
		glm::vec3(28.0f, 1.0f, 18.0f),
		90.0f,
		0.0f,
		0.0f,
		glm::vec3(0.0f, 1.8f, -11.5f));

	SetShaderLighting(false);
	SetShaderTexture("starfield");
	SetTextureUVScale(1.0f, 1.0f);
	m_basicMeshes->DrawPlaneMesh();
}

/***********************************************************
 *  DrawPlanetCloudLayer()
 *
 *  The reference image shows the battle over a bright clouded
 *  planet. The temporary flat plane was replaced by a large
 *  textured sphere to create a more cinematic horizon.
 ***********************************************************/
void SceneManager::DrawPlanetCloudLayer()
{
	// Large sphere replaces the temporary flat base plane. The center is placed well
	// below the ships so the visible upper edge reads as a planet horizon.
	SetTransformations(
		glm::vec3(5.80f, 5.80f, 5.80f),
		16.0f,
		-32.0f,
		0.0f,
		glm::vec3(-0.35f, -6.95f, 1.45f));

	SetShaderLighting(true);
	SetShaderMaterial("planetMaterial");
	SetShaderTexture("planetClouds");
	SetTextureUVScale(1.0f, 1.0f);
	m_basicMeshes->DrawSphereMesh();
}

/***********************************************************
 *  DrawSceneBasePlane()
 *
 *  Kept as a scene helper name for compatibility. The actual
 *  ground/planet object is now a sphere; the plane primitive
 *  still appears in the starfield backdrop.
 ***********************************************************/
void SceneManager::DrawSceneBasePlane()
{
	DrawPlanetCloudLayer();
}

/***********************************************************
 *  DrawSaucerSection()
 *
 *  The saucer is built from flattened spheres to keep the
 *  triangle count low while still reading as a Constitution
 *  class primary hull.
 ***********************************************************/
void SceneManager::DrawSaucerSection()
{
	// Main saucer.
	SetCruiserPartTransform(
		glm::vec3(0.80f, 0.09f, 0.80f),
		glm::vec3(0.0f, 0.0f, 0.0f),
		glm::vec3(0.55f, 0.26f, 0.0f));
	SetShaderLighting(true);
	SetShaderMaterial("federationHull");
	SetShaderTexture("shipHull");
	SetTextureUVScale(2.0f, 2.0f);
	m_basicMeshes->DrawSphereMesh();

	// Slightly smaller lower saucer layer.
	SetCruiserPartTransform(
		glm::vec3(0.50f, 0.045f, 0.50f),
		glm::vec3(0.0f, 0.0f, 0.0f),
		glm::vec3(0.55f, 0.20f, 0.0f));
	SetShaderMaterial("darkMetal");
	SetShaderTexture("darkHull");
	SetTextureUVScale(1.6f, 1.6f);
	m_basicMeshes->DrawSphereMesh();

	// Raised upper sensor layer.
	SetCruiserPartTransform(
		glm::vec3(0.38f, 0.045f, 0.38f),
		glm::vec3(0.0f, 0.0f, 0.0f),
		glm::vec3(0.55f, 0.35f, 0.0f));
	SetShaderMaterial("federationHull");
	SetShaderTexture("shipHull");
	SetTextureUVScale(1.2f, 1.2f);
	m_basicMeshes->DrawSphereMesh();
}

/***********************************************************
 *  DrawBridgeDome()
 ***********************************************************/
void SceneManager::DrawBridgeDome()
{
	SetCruiserPartTransform(
		glm::vec3(0.12f, 0.065f, 0.10f),
		glm::vec3(0.0f, 0.0f, 0.0f),
		glm::vec3(0.51f, 0.425f, 0.0f));
	SetShaderLighting(true);
	SetShaderMaterial("darkMetal");
	SetShaderTexture("darkHull");
	SetTextureUVScale(1.0f, 1.0f);
	m_basicMeshes->DrawSphereMesh();
}

/***********************************************************
 *  DrawEngineeringHull()
 *
 *  The secondary hull runs fore/aft along X. The cylinder mesh
 *  is rotated 90 degrees around Z because its height is its
 *  local Y axis.
 ***********************************************************/
void SceneManager::DrawEngineeringHull()
{
	// Main secondary hull.
	SetCruiserPartTransform(
		glm::vec3(0.20f, 1.18f, 0.20f),
		glm::vec3(0.0f, 0.0f, 90.0f),
		glm::vec3(0.01f, -0.36f, 0.0f));
	SetShaderLighting(true);
	SetShaderMaterial("federationHull");
	SetShaderTexture("shipHull");
	SetTextureUVScale(4.0f, 1.0f);
	m_basicMeshes->DrawCylinderMesh();

	// Aft cap.
	SetCruiserPartTransform(
		glm::vec3(0.15f, 0.075f, 0.20f),
		glm::vec3(0.0f, 0.0f, 90.0f),
		glm::vec3(-1.15f, -0.36f, 0.0f));
	SetShaderMaterial("darkMetal");
	SetShaderTexture("darkHull");
	SetTextureUVScale(1.0f, 1.0f);
	m_basicMeshes->DrawCylinderMesh();

	// Blue deflector dish on the nose of the secondary hull.
	SetCruiserPartTransform(
		glm::vec3(0.13f, 0.035f, 0.13f),
		glm::vec3(0.0f, 0.0f, 90.0f),
		glm::vec3(0.06f, -0.36f, 0.0f));
	SetShaderLighting(false);
	SetShaderTexture("blueDeflector");
	SetTextureUVScale(1.0f, 1.0f);
	m_basicMeshes->DrawCylinderMesh();
	SetShaderLighting(true);
}

/***********************************************************
 *  DrawSaucerNeck()
 ***********************************************************/
void SceneManager::DrawSaucerNeck()
{
	SetCruiserPartTransform(
		glm::vec3(0.175f, 0.52f, 0.105f),
		glm::vec3(0.0f, 0.0f, -22.0f),
		glm::vec3(-0.15f, -0.02f, 0.0f));
	SetShaderLighting(true);
	SetShaderMaterial("federationHull");
	SetShaderTexture("shipHull");
	SetTextureUVScale(1.2f, 2.0f);
	m_basicMeshes->DrawBoxMesh();
}

/***********************************************************
 *  DrawWarpNacelle()
 ***********************************************************/
void SceneManager::DrawWarpNacelle(glm::vec3 nacellePosition)
{
	float nacelleX = nacellePosition.x;
	float nacelleY = nacellePosition.y;
	float nacelleZ = nacellePosition.z;

	// Long warp engine cylinder.
	SetCruiserPartTransform(
		glm::vec3(0.115f, 1.61f, 0.115f),
		glm::vec3(0.0f, 0.0f, 90.0f),
		glm::vec3(nacelleX, nacelleY, nacelleZ));
	SetShaderLighting(true);
	SetShaderMaterial("federationHull");
	SetShaderTexture("shipHull");
	SetTextureUVScale(5.0f, 1.0f);
	m_basicMeshes->DrawCylinderMesh();

	// Orange Bussard collector.
	SetCruiserPartTransform(
		glm::vec3(0.105f, 0.105f, 0.105f),
		glm::vec3(0.0f, 0.0f, 0.0f),
		glm::vec3(nacelleX + 0.031f, nacelleY, nacelleZ));
	SetShaderLighting(false);
	SetShaderTexture("engineGlow");
	SetTextureUVScale(1.0f, 1.0f);
	m_basicMeshes->DrawSphereMesh();

	// Rear cap.
	SetCruiserPartTransform(
		glm::vec3(0.115f, 0.045f, 0.115f),
		glm::vec3(0.0f, 0.0f, 90.0f),
		glm::vec3(nacelleX - 1.65f, nacelleY, nacelleZ));
	SetShaderLighting(true);
	SetShaderMaterial("darkMetal");
	SetShaderTexture("darkHull");
	SetTextureUVScale(1.0f, 1.0f);
	m_basicMeshes->DrawCylinderMesh();
}

/***********************************************************
 *  DrawWarpPylon()
 ***********************************************************/
void SceneManager::DrawWarpPylon(float zOffset, float zRotationDegrees)
{
	float outwardTilt = (zOffset < 0.025f) ? -37.0f : 37.0f;
	float pylonSideOffset = (zOffset < 0.025f) ? -0.28f : 0.28f;

	SetCruiserPartTransform(
		glm::vec3(0.060f, 0.68f, 0.055f),
		glm::vec3(outwardTilt, 0.0f, zRotationDegrees),
		glm::vec3(-1.05f, 0.025f, pylonSideOffset));
	SetShaderLighting(true);
	SetShaderMaterial("federationHull");
	SetShaderTexture("shipHull");
	SetTextureUVScale(1.0f, 2.0f);
	m_basicMeshes->DrawBoxMesh();
}

/***********************************************************
 *  DrawDeflectorDish()
 ***********************************************************/
void SceneManager::DrawDeflectorDish()
{
	// Deflector is now part of DrawEngineeringHull().
}

/***********************************************************
 *  DrawConstitutionCruiser()
 *
 *  Builds one Federation cruiser. Calling this twice with the
 *  same scale but different coordinates places both ships in
 *  the battle layout.
 ***********************************************************/
void SceneManager::DrawConstitutionCruiser(glm::vec3 position, glm::vec3 rotationDegrees, float uniformScale)
{
	SetShipParentTransform(position, rotationDegrees, uniformScale);
	SetShaderLighting(true);

	DrawEngineeringHull();
	DrawSaucerNeck();
	DrawWarpPylon(-0.18f, 8.0f);
	DrawWarpPylon(0.18f, 8.0f);

	float nacelleX = -0.93f;
	float nacelleY = 0.405f;
	float nacelleSideDistance = 0.45f;
	DrawWarpNacelle(glm::vec3(nacelleX, nacelleY, -nacelleSideDistance));
	DrawWarpNacelle(glm::vec3(nacelleX, nacelleY, nacelleSideDistance));

	DrawSaucerSection();
	DrawBridgeDome();
}

/***********************************************************
 *  DrawKlingonWing()
 *
 *  Draws one swept D7 wing and engine pod. The side parameter
 *  is -1 for port and +1 for starboard. The wing plates and
 *  engine pylons intentionally overlap for coherence
 ***********************************************************/
void SceneManager::DrawKlingonWing(float side)
{
	float sideSign = (side < 0.0f) ? -1.0f : 1.0f;

	// Inner wing
	SetShipPartTransform(
		glm::vec3(0.88f, 0.060f, 0.36f),
		glm::vec3(0.0f, -10.0f * sideSign, 0.0f),
		glm::vec3(-0.58f, 0.000f, sideSign * 0.36f));
	SetShaderLighting(true);
	SetShaderMaterial("klingonHull");
	SetShaderTexture("klingonHull");
	SetTextureUVScale(2.0f, 1.0f);
	m_basicMeshes->DrawBoxMesh();

	// Outer swept wing plate
	SetShipPartTransform(
		glm::vec3(0.76f, 0.052f, 0.30f),
		glm::vec3(0.0f, -22.0f * sideSign, 0.0f),
		glm::vec3(-0.91f, -0.005f, sideSign * 0.70f));
	SetShaderMaterial("klingonHull");
	SetShaderTexture("klingonHull");
	SetTextureUVScale(1.8f, 0.8f);
	m_basicMeshes->DrawBoxMesh();

	// A narrow leading-edge plate fills the seam between the command hull and the wing.
	SetShipPartTransform(
		glm::vec3(0.62f, 0.035f, 0.10f),
		glm::vec3(0.0f, 20.0f * sideSign, 0.0f),
		glm::vec3(-0.26f, 0.040f, sideSign * 0.27f));
	SetShaderMaterial("klingonHull");
	SetShaderTexture("klingonHull");
	SetTextureUVScale(1.2f, 0.6f);
	m_basicMeshes->DrawBoxMesh();

	// Dark raised armor panel on top of the wing.
	SetShipPartTransform(
		glm::vec3(0.54f, 0.022f, 0.14f),
		glm::vec3(0.0f, -18.0f * sideSign, 0.0f),
		glm::vec3(-0.70f, 0.060f, sideSign * 0.47f));
	SetShaderMaterial("klingonDark");
	SetShaderTexture("klingonDark");
	SetTextureUVScale(1.3f, 1.0f);
	m_basicMeshes->DrawBoxMesh();

	// Engine mounting block
	SetShipPartTransform(
		glm::vec3(0.26f, 0.100f, 0.18f),
		glm::vec3(0.0f, -12.0f * sideSign, 0.0f),
		glm::vec3(-0.92f, -0.060f, sideSign * 0.80f));
	SetShaderMaterial("klingonDark");
	SetShaderTexture("klingonDark");
	SetTextureUVScale(1.0f, 0.7f);
	m_basicMeshes->DrawBoxMesh();

	// Long pylon overlaps both the wing plate and the engine pod.
	SetShipPartTransform(
		glm::vec3(0.58f, 0.060f, 0.12f),
		glm::vec3(0.0f, -8.0f * sideSign, 0.0f),
		glm::vec3(-1.17f, -0.095f, sideSign * 0.86f));
	SetShaderMaterial("klingonDark");
	SetShaderTexture("klingonDark");
	SetTextureUVScale(1.2f, 0.6f);
	m_basicMeshes->DrawBoxMesh();

	// Wing-tip engine pod
	SetShipPartTransform(
		glm::vec3(0.090f, 0.76f, 0.090f),
		glm::vec3(0.0f, 0.0f, 90.0f),
		glm::vec3(-1.25f, -0.100f, sideSign * 0.86f));
	SetShaderMaterial("klingonDark");
	SetShaderTexture("klingonDark");
	SetTextureUVScale(2.2f, 1.0f);
	m_basicMeshes->DrawCylinderMesh();

	// Front cap of the engine pod.
	SetShipPartTransform(
		glm::vec3(0.085f, 0.085f, 0.085f),
		glm::vec3(0.0f, 0.0f, 0.0f),
		glm::vec3(-0.85f, -0.100f, sideSign * 0.86f));
	SetShaderMaterial("klingonHull");
	SetShaderTexture("klingonHull");
	SetTextureUVScale(1.0f, 1.0f);
	m_basicMeshes->DrawSphereMesh();

	// Green aft engine exhaust glow.
	SetShipPartTransform(
		glm::vec3(0.085f, 0.085f, 0.085f),
		glm::vec3(0.0f, 0.0f, 0.0f),
		glm::vec3(-1.65f, -0.100f, sideSign * 0.86f));
	SetShaderLighting(false);
	SetShaderTexture("greenGlow");
	SetTextureUVScale(1.0f, 1.0f);
	m_basicMeshes->DrawSphereMesh();

	// Small rim around the engine exhaust
	SetShipPartTransform(
		glm::vec3(0.092f, 0.092f, 0.092f),
		glm::vec3(0.0f, 90.0f, 0.0f),
		glm::vec3(-1.67f, -0.100f, sideSign * 0.86f));
	SetShaderTexture("greenGlow");
	m_basicMeshes->DrawTorusMesh();
	SetShaderLighting(true);
}

/***********************************************************
 *  DrawKlingonD7Battlecruiser()
 *
 *  Low-polygon D7 built from sphere, cylinder, cone, box,
 *  torus, and plane primitives used elsewhere in the scene.
 ***********************************************************/
void SceneManager::DrawKlingonD7Battlecruiser(glm::vec3 position, glm::vec3 rotationDegrees, float uniformScale)
{
	SetShipParentTransform(position, rotationDegrees, uniformScale);
	SetShaderLighting(true);

	// Central aft hull, flattened vertically and widened laterally to anchor the wings.
	SetShipPartTransform(
		glm::vec3(0.58f, 0.15f, 0.42f),
		glm::vec3(0.0f, 0.0f, 0.0f),
		glm::vec3(-0.54f, 0.02f, 0.0f));
	SetShaderMaterial("klingonHull");
	SetShaderTexture("klingonHull");
	SetTextureUVScale(1.4f, 1.2f);
	m_basicMeshes->DrawSphereMesh();

	// A solid central deck joins both wings
	SetShipPartTransform(
		glm::vec3(0.84f, 0.055f, 0.84f),
		glm::vec3(0.0f, 0.0f, 0.0f),
		glm::vec3(-0.67f, -0.010f, 0.0f));
	SetShaderMaterial("klingonHull");
	SetShaderTexture("klingonHull");
	SetTextureUVScale(2.0f, 1.2f);
	m_basicMeshes->DrawBoxMesh();

	// Rear squared impulse engines
	SetShipPartTransform(
		glm::vec3(0.32f, 0.16f, 0.52f),
		glm::vec3(0.0f, 0.0f, 0.0f),
		glm::vec3(-1.10f, 0.015f, 0.0f));
	SetShaderMaterial("klingonDark");
	SetShaderTexture("klingonDark");
	SetTextureUVScale(1.0f, 1.0f);
	m_basicMeshes->DrawBoxMesh();

	// Raised dark spine along the aft hull.
	SetShipPartTransform(
		glm::vec3(0.62f, 0.060f, 0.18f),
		glm::vec3(0.0f, 0.0f, 0.0f),
		glm::vec3(-0.62f, 0.170f, 0.0f));
	SetShaderMaterial("klingonDark");
	SetShaderTexture("klingonDark");
	SetTextureUVScale(1.3f, 0.9f);
	m_basicMeshes->DrawBoxMesh();

	// Long neck/boom, pointed toward the command pod
	SetShipPartTransform(
		glm::vec3(0.075f, 1.62f, 0.075f),
		glm::vec3(0.0f, 0.0f, 90.0f),
		glm::vec3(0.40f, 0.052f, 0.0f));
	SetShaderMaterial("klingonDark");
	SetShaderTexture("klingonDark");
	SetTextureUVScale(3.0f, 1.0f);
	m_basicMeshes->DrawCylinderMesh();

	// Flattened upper armor plate
	SetShipPartTransform(
		glm::vec3(1.58f, 0.045f, 0.13f),
		glm::vec3(0.0f, 0.0f, 0.0f),
		glm::vec3(0.40f, 0.112f, 0.0f));
	SetShaderMaterial("klingonHull");
	SetShaderTexture("klingonHull");
	SetTextureUVScale(2.2f, 0.7f);
	m_basicMeshes->DrawBoxMesh();

	// Lower boom plate for thickness from underside views.
	SetShipPartTransform(
		glm::vec3(1.40f, 0.040f, 0.10f),
		glm::vec3(0.0f, 0.0f, 0.0f),
		glm::vec3(0.32f, -0.008f, 0.0f));
	SetShaderMaterial("klingonDark");
	SetShaderTexture("klingonDark");
	SetTextureUVScale(2.0f, 0.6f);
	m_basicMeshes->DrawBoxMesh();

	// Rear collar blends the neck into the main hull.
	SetShipPartTransform(
		glm::vec3(0.15f, 0.11f, 0.16f),
		glm::vec3(0.0f, 0.0f, 0.0f),
		glm::vec3(-0.30f, 0.052f, 0.0f));
	SetShaderMaterial("klingonHull");
	SetShaderTexture("klingonHull");
	SetTextureUVScale(1.0f, 1.0f);
	m_basicMeshes->DrawSphereMesh();

	// Forward collar connecting the neck to the command pod.
	SetShipPartTransform(
		glm::vec3(0.18f, 0.12f, 0.17f),
		glm::vec3(0.0f, 0.0f, 0.0f),
		glm::vec3(1.12f, 0.060f, 0.0f));
	SetShaderMaterial("klingonHull");
	SetShaderTexture("klingonHull");
	SetTextureUVScale(1.0f, 1.0f);
	m_basicMeshes->DrawSphereMesh();

	// Short bridge connector
	SetShipPartTransform(
		glm::vec3(0.32f, 0.075f, 0.16f),
		glm::vec3(0.0f, 0.0f, 0.0f),
		glm::vec3(1.22f, 0.055f, 0.0f));
	SetShaderMaterial("klingonHull");
	SetShaderTexture("klingonHull");
	SetTextureUVScale(1.0f, 0.8f);
	m_basicMeshes->DrawBoxMesh();

	// Forward command pod.
	SetShipPartTransform(
		glm::vec3(0.32f, 0.15f, 0.23f),
		glm::vec3(0.0f, 0.0f, 0.0f),
		glm::vec3(1.40f, 0.060f, 0.0f));
	SetShaderMaterial("klingonHull");
	SetShaderTexture("klingonHull");
	SetTextureUVScale(1.0f, 1.0f);
	m_basicMeshes->DrawSphereMesh();

	// Pointed forward nose cuz why not.
	SetShipPartTransform(
		glm::vec3(0.105f, 0.26f, 0.105f),
		glm::vec3(0.0f, 0.0f, -90.0f),
		glm::vec3(1.72f, 0.060f, 0.0f));
	SetShaderMaterial("klingonDark");
	SetShaderTexture("klingonDark");
	SetTextureUVScale(1.0f, 1.0f);
	m_basicMeshes->DrawConeMesh();

	// Raised bridge deck
	SetShipPartTransform(
		glm::vec3(0.085f, 0.052f, 0.078f),
		glm::vec3(0.0f, 0.0f, 0.0f),
		glm::vec3(1.36f, 0.205f, 0.0f));
	SetShaderMaterial("klingonDark");
	SetShaderTexture("klingonDark");
	SetTextureUVScale(1.0f, 1.0f);
	m_basicMeshes->DrawSphereMesh();

	// Small green forward disruptor emitters.
	for (int i = -1; i <= 1; i += 2)
	{
		SetShipPartTransform(
			glm::vec3(0.026f, 0.026f, 0.026f),
			glm::vec3(0.0f, 0.0f, 0.0f),
			glm::vec3(1.76f, 0.050f, 0.040f * i));
		SetShaderLighting(false);
		SetShaderTexture("greenGlow");
		SetTextureUVScale(1.0f, 1.0f);
		m_basicMeshes->DrawSphereMesh();
		SetShaderLighting(true);
	}

	DrawKlingonWing(-1.0f);
	DrawKlingonWing(1.0f);
}

/***********************************************************
 *  DrawPhotonTorpedo()
 *
 *  Renders a compact projectile with a bright core and a small
 *  translucent outer shell. The actual scene lighting for each
 *  torpedo is handled in SetupSceneLights().
 ***********************************************************/
void SceneManager::DrawPhotonTorpedo(glm::vec3 position, float radius)
{
	// Bright core.
	SetTransformations(
		glm::vec3(radius, radius, radius),
		0.0f,
		0.0f,
		0.0f,
		position);
	SetShaderLighting(false);
	SetShaderTexture("photonGlow");
	SetTextureUVScale(1.0f, 1.0f);
	m_basicMeshes->DrawSphereMesh();

	// Tight red-orange halo. This is intentionally small so it mimics a
	// photon torpedo glow instead of looking like a large balloon around the projectile.
	SetTransformations(
		glm::vec3(radius * 1.55f, radius * 1.55f, radius * 1.55f),
		0.0f,
		0.0f,
		0.0f,
		position);
	SetShaderColor(1.00f, 0.22f, 0.025f, 0.32f);
	m_basicMeshes->DrawSphereMesh();
}

/***********************************************************
 *  DrawPhotonTorpedoTrail()
 *
 *  The scene uses four torpedoes: two fired from
 *  the front of the foreground Klingon D7 and two fired from
 *  a Fed ship grouped along a firing path
 ***********************************************************/
void SceneManager::DrawPhotonTorpedoTrail()
{
	// Group A: direct line between the upper Federation ship and the large foreground Klingon D7.
	DrawPhotonTorpedo(TORPEDO_A1, 0.040f);
	DrawPhotonTorpedo(TORPEDO_A2, 0.040f);

	// Group B: direct line between the lower-left Federation ship and the upper-right Klingon D7.
	DrawPhotonTorpedo(TORPEDO_B1, 0.040f);
	DrawPhotonTorpedo(TORPEDO_B2, 0.040f);

	SetShaderLighting(true);
}

/***********************************************************
 *  RenderScene()
 *
 *  Final Star Trek battle scene
 ***********************************************************/
void SceneManager::RenderScene()
{
	DrawStarfieldBackdrop();
	DrawSceneBasePlane();

	const float shipScale = 1.15f;

	// Large foreground Klingon ship
	DrawKlingonD7Battlecruiser(
		glm::vec3(-3.10f, 0.25f, 3.10f),
		glm::vec3(-8.0f, 28.0f, -38.0f),
		shipScale);

	// Main Federation ship, high and near the center
	DrawConstitutionCruiser(
		glm::vec3(1.10f, 1.70f, 0.20f),
		glm::vec3(16.0f, -38.0f, 12.0f),
		shipScale);

	// Lower-left Federation ship.
	DrawConstitutionCruiser(
		glm::vec3(-4.75f, -1.10f, 1.10f),
		glm::vec3(7.0f, -55.0f, -12.0f),
		shipScale);

	// Distant upper-right Klingon D7, farther back in Z.
	DrawKlingonD7Battlecruiser(
		glm::vec3(4.25f, 2.05f, -2.55f),
		glm::vec3(10.0f, 140.0f, 18.0f),
		shipScale);

	DrawPhotonTorpedoTrail();
}