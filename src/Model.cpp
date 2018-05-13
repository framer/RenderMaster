#include "Model.h"
#include "Core.h"

extern Core *_pCore;
DEFINE_DEBUG_LOG_HELPERS(_pCore)
DEFINE_LOG_HELPERS(_pCore)

Model::Model(std::vector<ICoreMesh *>& meshes) : _meshes(meshes)
{
}

Model::~Model()
{
}

API Model::GetMesh(OUT ICoreMesh  **pMesh, uint idx)
{
	*pMesh = _meshes.at(idx);
	return S_OK;
}

API Model::GetNumberOfMesh(OUT uint *number)
{
	*number = (uint)_meshes.size();
	return S_OK;
}

API Model::Free()
{
	for (auto *mesh : _meshes)
		mesh->Free();

	standard_free_and_delete(this, std::function<void()>(), _pCore);

	return S_OK;
}

API Model::GetType(OUT RES_TYPE *type)
{
	*type = RES_TYPE::MODEL;
	return S_OK;
}


