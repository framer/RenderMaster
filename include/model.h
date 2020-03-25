#pragma once
#include "gameobject.h"
#include "mesh.h"
#include "resource_manager.h"


class Model final : public GameObject
{
	StreamPtr<Mesh> meshPtr;
	vec3 meshCeneter;
	Material *mat_{nullptr};

protected:
	virtual void Copy(GameObject *original) override;
	virtual void SaveYAML(void *yaml) override;
	virtual void LoadYAML(void *yaml) override;

public:
	Model();
	Model(StreamPtr<Mesh> mesh);

	auto DLLEXPORT virtual GetMesh() -> Mesh*;
	auto DLLEXPORT virtual GetMeshPath() -> const char*;
	auto DLLEXPORT virtual SetMaterial(Material *mat) -> void { mat_ = mat; }
	auto DLLEXPORT virtual GetMaterial() -> Material* { return mat_; }
	auto DLLEXPORT virtual GetWorldCenter() -> vec3;

	// GameObject
	auto DLLEXPORT virtual Clone() -> GameObject* override;
};
