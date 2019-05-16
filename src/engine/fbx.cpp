#include "pch.h"
#include "core.h"
#include "console.h"
#include "filesystem.h"
#include "resource_manager.h"
#include "fbx.h"
#include <fbxsdk.h>

#ifdef IOS_REF
#undef  IOS_REF
#define IOS_REF (*(pManager->GetIOSettings()))
#endif

const int fbxDebug = 1;

bool _FBX_load_scene(FbxManager* pManager, FbxDocument* pScene, const char* pFilename);
void _FBX_load_scene_hierarchy(FbxScene* pScene, const char *pFullPath, const char *pRelativePath);
void _FBX_load_node(FbxNode* pNode, int pDepth, const char *pFullPath, const char *pRelativePath, int& meshes);
void _FBX_load_mesh(FbxMesh *pMesh, FbxNode *pNode, const char *pFullPath, const char *pRelativePath);
void _FBX_load_node_transform(FbxNode* pNode, const char *str);


void importFbx(const char * path)
{
	FbxManager* lSdkManager = NULL;
	FbxScene* lScene = NULL;

	if (fbxDebug) Log("Initializing FBX SDK...");

	lSdkManager = FbxManager::Create();

	if (!lSdkManager)
	{
		if (fbxDebug) LogCritical("Error: Unable to create FBX Manager!");
		return;
	}

	if (fbxDebug) Log("Autodesk FBX SDK version %s", lSdkManager->GetVersion());

	FbxIOSettings* ios = FbxIOSettings::Create(lSdkManager, IOSROOT);
	lSdkManager->SetIOSettings(ios);

	FbxString lPath = FbxGetApplicationDirectory();
	lSdkManager->LoadPluginsDirectory(lPath.Buffer());

	lScene = FbxScene::Create(lSdkManager, "My Scene");

	if (!lScene)
	{
		LogCritical("Error: Unable to create FBX scene!");
		return;
	}

	FbxString lFilePath(path);

	Log("Loading file: %s", lFilePath.Buffer());

	bool lResult = _FBX_load_scene(lSdkManager, lScene, lFilePath.Buffer());

	if (!lResult)
		LogCritical("An error occurred while loading the scene");
	else
		_FBX_load_scene_hierarchy(lScene, path, path);

	if (fbxDebug) Log("Destroying FBX SDK...");
	if (lSdkManager) lSdkManager->Destroy();
}

bool _FBX_load_scene(FbxManager* pManager, FbxDocument* pScene, const char* pFilename)
{
	int lFileMajor, lFileMinor, lFileRevision;
	int lSDKMajor, lSDKMinor, lSDKRevision;
	int lAnimStackCount;
	bool lStatus;

	FbxManager::GetFileFormatVersion(lSDKMajor, lSDKMinor, lSDKRevision);

	// Create an importer.
	FbxImporter* lImporter = FbxImporter::Create(pManager, "");

	const bool lImportStatus = lImporter->Initialize(pFilename, -1, pManager->GetIOSettings());
	lImporter->GetFileVersion(lFileMajor, lFileMinor, lFileRevision);

	if (!lImportStatus)
	{
		FbxString error = lImporter->GetStatus().GetErrorString();

		LogCritical("Call to FbxImporter::Initialize() failed.");
		LogCritical("Error returned: %s", error.Buffer());

		if (lImporter->GetStatus().GetCode() == FbxStatus::eInvalidFileVersion)
		{
			Log("FBX file format version for this FBX SDK is %d.%d.%d", lSDKMajor, lSDKMinor, lSDKRevision);
			Log("FBX file format version for file '%s' is %d.%d.%d", pFilename, lFileMajor, lFileMinor, lFileRevision);
		}

		return false;
	}

	if (fbxDebug) Log("FBX file format version for this FBX SDK is %d.%d.%d", lSDKMajor, lSDKMinor, lSDKRevision);

	if (lImporter->IsFBX())
	{
		if (fbxDebug) Log("FBX file format version for file '%s' is %d.%d.%d", pFilename, lFileMajor, lFileMinor, lFileRevision);

		// From this point, it is possible to access animation stack information without
		// the expense of loading the entire file.
		lAnimStackCount = lImporter->GetAnimStackCount();

		if (fbxDebug) Log("Animation Stack Information:");
		if (fbxDebug) Log("Number of Animation Stacks: %d", lAnimStackCount);
		if (fbxDebug) Log("Current Animation Stack: \"%s\"", lImporter->GetActiveAnimStackName().Buffer());

		// Set the import states. By default, the import states are always set to 
		// true. The code below shows how to change these states.
		IOS_REF.SetBoolProp(IMP_FBX_MATERIAL, true);
		IOS_REF.SetBoolProp(IMP_FBX_TEXTURE, true);
		IOS_REF.SetBoolProp(IMP_FBX_LINK, true);
		IOS_REF.SetBoolProp(IMP_FBX_SHAPE, true);
		IOS_REF.SetBoolProp(IMP_FBX_GOBO, true);
		IOS_REF.SetBoolProp(IMP_FBX_ANIMATION, true);
		IOS_REF.SetBoolProp(IMP_FBX_GLOBAL_SETTINGS, true);
	}

	// Import the scene.
	lStatus = lImporter->Import(pScene);

	if (lStatus == false && lImporter->GetStatus().GetCode() == FbxStatus::ePasswordError)
		LogCritical("No support entering password!");

	lImporter->Destroy();

	return lStatus;
}

void _FBX_load_scene_hierarchy(FbxScene *pScene, const char *pFullPath, const char *path)
{
	FbxString lString;

	if (fbxDebug) Log("Scene hierarchy:");

	FbxNode* lRootNode = pScene->GetRootNode();
	int meshes = 0;
	_FBX_load_node(lRootNode, 0, pFullPath, path, meshes);
}

void _FBX_load_node(FbxNode* pNode, int depth, const char *fullPath, const char *path, int& meshes)
{
	FbxString lString;
	FbxNodeAttribute* node = pNode->GetNodeAttribute();
	FbxNodeAttribute::EType lAttributeType = FbxNodeAttribute::eUnknown;
	if (node) lAttributeType = node->GetAttributeType();

	switch (lAttributeType)
	{
		case FbxNodeAttribute::eMesh:		if (meshes==0) _FBX_load_mesh((FbxMesh*)pNode->GetNodeAttribute(), pNode, fullPath, path); meshes++; break; // load only first mesh
		case FbxNodeAttribute::eMarker:		Log(("(eMarker) " + lString + pNode->GetName()).Buffer()); break;
		case FbxNodeAttribute::eSkeleton:	Log(("(eSkeleton) " + lString + pNode->GetName()).Buffer()); break;
		case FbxNodeAttribute::eNurbs:		Log(("(eNurbs) " + lString + pNode->GetName()).Buffer()); break;
		case FbxNodeAttribute::ePatch:		Log(("(ePatch) " + lString + pNode->GetName()).Buffer()); break;
		case FbxNodeAttribute::eCamera:		_FBX_load_node_transform(pNode, ("(eCamera) " + lString + pNode->GetName()).Buffer()); break;
		case FbxNodeAttribute::eLight:		Log(("(eLight) " + lString + pNode->GetName()).Buffer()); break;
		case FbxNodeAttribute::eLODGroup:	Log(("(eLODGroup) " + lString + pNode->GetName()).Buffer()); break;
		default:							_FBX_load_node_transform(pNode, ("(unknown!) " + lString + pNode->GetName()).Buffer()); break;
	}

	int childs = pNode->GetChildCount();
	if (childs)
	{
		if (fbxDebug) Log("for node=%s childs=%i", pNode->GetName(), childs);
		for (int i = 0; i < childs; i++)
		{
			_FBX_load_node(pNode->GetChild(i), depth + 1, fullPath, path, meshes);
		}
	}
}

void add_tabs(FbxString& buff, int tabs)
{
	for (int i = 0; i < tabs; i++)
		buff += " ";
}

void _FBX_load_mesh(FbxMesh *pMesh, FbxNode *pNode, const char *fullPath, const char *path)
{
	const int control_points_count = pMesh->GetControlPointsCount();
	const int polygon_count = pMesh->GetPolygonCount();
	const int normal_element_count = pMesh->GetElementNormalCount();
	const int uv_layer_count = pMesh->GetElementUVCount();
	const int tangent_layers_count = pMesh->GetElementTangentCount();
	const int binormal_layers_count = pMesh->GetElementBinormalCount();
	const int color_layers_count = pMesh->GetElementVertexColorCount();

	const int vertecies = polygon_count * 3;

	const int is_normals = normal_element_count > 0;
	const int is_uv = uv_layer_count > 0;

	//pMesh->GenerateNormals();

	// TODO: binormal, tangent, color
	const int is_tangent = 0; //tangent_layers_count > 0;
	const int is_binormal = 0; //binormal_layers_count > 0;
	const int is_color = 0; //color_layers_count > 0;

	vector<float> data;
	data.reserve(vertecies * (4 + is_normals * 4 + is_uv * 2));
	
	FbxVector4 tr = pNode->EvaluateGlobalTransform().GetT();
	FbxVector4 rot = pNode->EvaluateGlobalTransform().GetR();
	FbxVector4 sc = pNode->EvaluateGlobalTransform().GetS();

	if (fbxDebug)
		Log("(eMesh) %-10.10s T=(%.1f %.1f %.1f) R=(%.1f %.1f %.1f) S=(%.1f %.1f %.1f) CP=%5d POLYS=%5d NORMAL=%d UV=%d TANG=%d BINORM=%d", 
		pNode->GetName(),
		tr[0], tr[1], tr[2], rot[0], rot[1], rot[2], sc[0], sc[1], sc[2],
		control_points_count, polygon_count, normal_element_count, uv_layer_count, tangent_layers_count, binormal_layers_count);

	FbxVector4* control_points_array = pMesh->GetControlPoints();
	FbxGeometryElementNormal* pNormals = pMesh->GetElementNormal(0);

	FbxGeometryElementUV *pUV = pMesh->GetElementUV(0);

	int vertexId = 0;
	int vertexes = 0;

	for (int i = 0; i < polygon_count; i++)
	{
		int polygon_size = pMesh->GetPolygonSize(i);

		for (int t = 0; t < polygon_size - 2; t++) // triangulate large polygons
			for (int j = 0; j < 3; j++)
			{
				int localVertId = t + j;
				if (j == 0)
					localVertId = 0;

				int ctrl_point_idx = pMesh->GetPolygonVertex(i, localVertId);

				// position
				FbxVector4 lCurrentVertex = control_points_array[ctrl_point_idx];

				data.push_back((float)lCurrentVertex[0]);
				data.push_back((float)lCurrentVertex[1]);
				data.push_back((float)lCurrentVertex[2]);
				data.push_back(1.0f);

				// normal
				if (is_normals)
				{
					FbxVector4 normal_fbx;

					switch (pNormals->GetMappingMode())
					{
						case FbxGeometryElement::eByControlPoint:
						{
							switch (pNormals->GetReferenceMode())
							{
								case FbxGeometryElement::eDirect:
									normal_fbx = pNormals->GetDirectArray().GetAt(ctrl_point_idx);
									//pMesh->GetPolygonVertexNormal(i, localVertId, normal_fbx);
								break;

								case FbxGeometryElement::eIndexToDirect:
								{
									int id = pNormals->GetIndexArray().GetAt(ctrl_point_idx);
									normal_fbx = pNormals->GetDirectArray().GetAt(id);
								}
								break;

								default: break;
							}
						}
						break;

						case FbxGeometryElement::eByPolygonVertex:
						{
							switch (pNormals->GetReferenceMode())
							{
							case FbxGeometryElement::eDirect:
								normal_fbx = pNormals->GetDirectArray().GetAt(vertexId + localVertId);
								break;

							case FbxGeometryElement::eIndexToDirect:
							{
								int id = pNormals->GetIndexArray().GetAt(vertexId + localVertId);
								normal_fbx = pNormals->GetDirectArray().GetAt(id);
							}
							break;

							default: break;
							}
						}
						break;

						default: assert(false);
							break;
					}

					data.push_back((float)normal_fbx[0]);
					data.push_back((float)normal_fbx[1]);
					data.push_back((float)normal_fbx[2]);
					data.push_back(0.0f);
				}

				// uv
				if (is_uv)
				{
					FbxVector2 uv_fbx;

					switch (pUV->GetMappingMode())
					{
					case FbxGeometryElement::eByControlPoint:
					{
						switch (pUV->GetReferenceMode())
						{
						case FbxGeometryElement::eDirect:
							uv_fbx = pUV->GetDirectArray().GetAt(ctrl_point_idx);
							break;

						case FbxGeometryElement::eIndexToDirect:
						{
							int id = pUV->GetIndexArray().GetAt(ctrl_point_idx);
							uv_fbx = pUV->GetDirectArray().GetAt(id);
						}
						break;

						default: break;
						}
					}
					break;

					case FbxGeometryElement::eByPolygonVertex:
					{
						switch (pUV->GetReferenceMode())
						{
						case FbxGeometryElement::eDirect:
							uv_fbx = pUV->GetDirectArray().GetAt(vertexId + localVertId);
							break;

						case FbxGeometryElement::eIndexToDirect:
						{
							int id = pUV->GetIndexArray().GetAt(vertexId + localVertId);
							uv_fbx = pUV->GetDirectArray().GetAt(id);
						}
						break;

						default: break;
						}
					}
					break;

					default: assert(false);
						break;
					}

					data.push_back((float)uv_fbx[0]);
					data.push_back((float)uv_fbx[1]);
				}
				vertexes++;
			}

		vertexId += polygon_size;
	}

	MeshHeader header{};
	header.magic[0] = 'M'; 
	header.magic[0] = 'S'; 
	header.magic[0] = 'H'; 
	header.attributes = ((vertecies > 0) << 0)
		| (is_normals << 1)
		| (is_uv << 2)
		| (is_tangent << 3)
		| (is_binormal << 4)
		| (is_color << 5);

	header.numberOfVertex = vertexes;

	uint32_t bytes = 0;

	int stride = sizeof(vec4) +
		is_normals * sizeof(vec4) +
		is_uv * sizeof(vec2) +
		is_tangent * sizeof(vec4) +
		is_binormal * sizeof(vec4) +
		is_color * sizeof(vec4);

	header.positionOffset = 0;
	header.positionStride = stride;
	bytes += 1 * sizeof(vec4);

	header.normalOffset = is_normals * bytes;
	header.normalStride = stride;
	bytes += is_normals * sizeof(vec4);

	header.uvOffset = is_uv * bytes;
	header.uvStride = stride;
	bytes += is_uv * sizeof(vec2);

	header.tangentOffset = is_tangent * bytes;
	header.tangentStride = stride;
	bytes += is_tangent * sizeof(vec4);

	header.binormalOffset = is_binormal * bytes;
	header.binormalStride = stride;
	bytes += is_binormal * sizeof(vec4);
	 
	header.colorOffset = is_color * bytes;
	header.colorStride = stride;
	bytes += is_color * sizeof(vec4);

	string p = RES_MAN->GetImportMeshDir() + "\\teapot.mesh";

	File f = FS->OpenFile(p.c_str(), FILE_OPEN_MODE::WRITE | FILE_OPEN_MODE::BINARY);

	f.Write(reinterpret_cast<uint8*>(&header), sizeof(MeshHeader));
	f.Write(reinterpret_cast<uint8*>(&data[0]), data.size() * sizeof(float));
}

void _FBX_load_node_transform(FbxNode* pNode, const char *str)
{
	FbxVector4 tr = pNode->EvaluateGlobalTransform().GetT();
	FbxVector4 rot = pNode->EvaluateGlobalTransform().GetR();
	FbxVector4 sc = pNode->EvaluateGlobalTransform().GetS();

	if (fbxDebug)
		Log("%s T=(%.1f %.1f %.1f) R=(%.1f %.1f %.1f) S=(%.1f %.1f %.1f)", str, tr[0], tr[1], tr[2], rot[0], rot[1], rot[2], sc[0], sc[1], sc[2]);
}
