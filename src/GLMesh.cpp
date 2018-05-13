#include "GLMesh.h"
#include "Core.h"

extern Core *_pCore;
DEFINE_DEBUG_LOG_HELPERS(_pCore)
DEFINE_LOG_HELPERS(_pCore)

GLMesh::GLMesh(GLuint VAO, GLuint VBO, GLuint IBO, uint vertexNumber, uint indexNumber, MESH_INDEX_FORMAT indexFormat, VERTEX_TOPOLOGY mode, INPUT_ATTRUBUTE a):
	_VAO(VAO), _VBO(VBO), _IBO(IBO),
	_number_of_vertices(vertexNumber), _number_of_indicies(indexNumber), _index_presented(indexFormat != MESH_INDEX_FORMAT::NOTHING), _topology(mode), _attributes(a)
{
}

API GLMesh::GetNumberOfVertex(OUT uint *vertex)
{
	*vertex = _number_of_vertices;
	return S_OK;
}

API GLMesh::GetAttributes(OUT INPUT_ATTRUBUTE *attribs)
{
	*attribs = _attributes;
	return S_OK;
}

API GLMesh::GetVertexTopology(OUT VERTEX_TOPOLOGY *topology)
{
	*topology = _topology;
	return S_OK;
}

API GLMesh::Free()
{
	auto free_gl_mesh = [&]() -> void
	{
		if (_index_presented) glDeleteBuffers(1, &_IBO);
		glDeleteBuffers(1, &_VBO);
		glDeleteVertexArrays(1, &_VAO);
	};

	standard_free_and_delete(this, free_gl_mesh, _pCore);

	return S_OK;
}

API GLMesh::GetType(OUT RES_TYPE *type)
{
	*type = RES_TYPE::CORE_MESH;
	return S_OK;
}
