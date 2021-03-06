#include "pch.h"
#include "dx11shader.h"
#include "dx11corerender.h"
#include "core.h"
#include "console.h"
#include "dx_objects.inl"

extern vector<ConstantBuffer> ConstantBufferPool;

static ID3D11DeviceContext* getContext()
{
	DX11CoreRender *dxRender = static_cast<DX11CoreRender*>(CORE_RENDER);
	return dxRender->getContext();
}
static ID3D11Device* getDevice()
{
	DX11CoreRender *dxRender = static_cast<DX11CoreRender*>(CORE_RENDER);
	return dxRender->getDevice();
}

void DX11Shader::initSubShader(ShaderInitData& data, SHADER_TYPE type)
{
	switch (type)
	{
		case SHADER_TYPE::SHADER_VERTEX: v.pointer = (ID3D11VertexShader *)data.pointer; break;
		case SHADER_TYPE::SHADER_GEOMETRY: g.pointer = (ID3D11GeometryShader *)data.pointer; break;
		case SHADER_TYPE::SHADER_FRAGMENT:  f.pointer = (ID3D11PixelShader *)data.pointer; break;
		case SHADER_TYPE::SHADER_COMPUTE:  c.pointer = (ID3D11ComputeShader *)data.pointer; break;
	}

	ID3D11ShaderReflection* reflection = nullptr;
	D3DReflect(data.bytecode, data.size, IID_ID3D11ShaderReflection, (void**)&reflection);

	D3D11_SHADER_DESC shaderDesc;
	reflection->GetDesc(&shaderDesc);

	// each Constant Buffer
	for(unsigned int i = 0; i < shaderDesc.ConstantBuffers; ++i)
	{
		ID3D11ShaderReflectionConstantBuffer* buffer = reflection->GetConstantBufferByIndex(i);

		D3D11_SHADER_BUFFER_DESC bufferDesc;
		buffer->GetDesc(&bufferDesc);

		vector<ConstantBuffer::Parameter> cbParameters;

		// each parameters
		for (uint j = 0; j < bufferDesc.Variables; j++)
		{
			ID3D11ShaderReflectionVariable *var = buffer->GetVariableByIndex(j);
			D3D11_SHADER_VARIABLE_DESC varDesc;
			var->GetDesc(&varDesc);

			ConstantBuffer::Parameter p;
			p.name = varDesc.Name;
			p.bytes = varDesc.Size;
			p.offset = varDesc.StartOffset;

			cbParameters.push_back(p);
		}

		// find existing buffer with same memory layout
		int indexFound = -1;
		{
			for (int j = 0; indexFound == -1 && j < ConstantBufferPool.size(); j++)
			{
				bool isEqual = true;

				//if (ConstantBufferPool[j].name != bufferDesc.Name)
				//	isEqual = false;

				if (ConstantBufferPool[j].bytes != bufferDesc.Size)
					isEqual = false;

				if (ConstantBufferPool[j].parameters.size() != cbParameters.size())
					isEqual = false;

				for (int k = 0; isEqual && k < cbParameters.size(); k++)
				{
					if (ConstantBufferPool[j].parameters[k].name != cbParameters[k].name ||
						ConstantBufferPool[j].parameters[k].bytes != cbParameters[k].bytes ||
						ConstantBufferPool[j].parameters[k].offset != cbParameters[k].offset)
					{
						isEqual = 0;
					}
				}

				if (isEqual)
					indexFound = j;
			}
		}

		if (indexFound == -1) // create new constant buffer
		{
			indexFound = (int)ConstantBufferPool.size();

			WRL::ComPtr<ID3D11Buffer> buffer;
		
			uint size = (bufferDesc.Size + 15) & ~15; // make byte width multiplied by 16
		
			D3D11_BUFFER_DESC desc{};
			desc.Usage = D3D11_USAGE_DYNAMIC;
			desc.ByteWidth = size;
			desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
			desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

			ThrowIfFailed(getDevice()->CreateBuffer(&desc, nullptr, buffer.GetAddressOf()));

			ConstantBufferPool.emplace_back(buffer, size, bufferDesc.Name, cbParameters);
		}

		switch (type)
		{
			case SHADER_TYPE::SHADER_VERTEX:	v.buffers.push_back(indexFound); break;
			case SHADER_TYPE::SHADER_GEOMETRY:	g.buffers.push_back(indexFound); break;
			case SHADER_TYPE::SHADER_FRAGMENT:	f.buffers.push_back(indexFound); break;
			case SHADER_TYPE::SHADER_COMPUTE:	c.buffers.push_back(indexFound); break;
		};

		for (int i = 0; i < cbParameters.size(); i++)
			parameters[cbParameters[i].name] = { (int)indexFound, (int)i };

	}
}

DX11Shader::DX11Shader(ShaderInitData& vs, ShaderInitData& fs, ShaderInitData& gs)
{
	initSubShader(vs, SHADER_TYPE::SHADER_VERTEX);
	initSubShader(fs, SHADER_TYPE::SHADER_FRAGMENT);

	if (gs.pointer)
		initSubShader(gs, SHADER_TYPE::SHADER_GEOMETRY);
}

DX11Shader::DX11Shader(ShaderInitData& cs)
{
	initSubShader(cs, SHADER_TYPE::SHADER_COMPUTE);
}

DX11Shader::~DX11Shader()
{
	if (v.pointer)	{ v.pointer->Release();	v.pointer = nullptr; }
	if (f.pointer)	{ f.pointer->Release();	f.pointer = nullptr; }
	if (g.pointer)	{ g.pointer->Release();	g.pointer = nullptr; }
	if (c.pointer)	{ c.pointer->Release();	c.pointer = nullptr; }
}

void DX11Shader::bind()
{
	ID3D11DeviceContext *ctx = getContext();

	ctx->VSSetShader(vs(), nullptr, 0);
	ctx->PSSetShader(fs(), nullptr, 0);
	ctx->GSSetShader(gs(), nullptr, 0);
	ctx->CSSetShader(cs(), nullptr, 0);

	ID3D11Buffer *pointers[128];

	#define BUND_CONSTANT_BUFFERS(PREFIX, IDX_VEC) \
		{ \
			for (size_t i = 0; i < IDX_VEC.size(); i++) \
			{ \
				ConstantBuffer& cb = ConstantBufferPool[IDX_VEC[i]]; \
				pointers[i] = cb.buffer.Get(); \
			} \
			ctx->PREFIX##SetConstantBuffers(0, (uint)IDX_VEC.size(), pointers); \
		}

	if (vs())
	BUND_CONSTANT_BUFFERS(VS, v.buffers)

	if (fs())
	BUND_CONSTANT_BUFFERS(PS, f.buffers)

	if (gs())
	BUND_CONSTANT_BUFFERS(GS, g.buffers)

	if (cs())
	BUND_CONSTANT_BUFFERS(CS, c.buffers)
}

void DX11Shader::setParameter(std::string_view name, const void *data)
{
	auto it = parameters.find(name);
	if (it == parameters.end())
	{
		LogWarning("DX11Shader::setParameter() unable find parameter \"%s\"", name.data());
		return;
	}

	Parameter &p = it->second;

	if (p.bufferIndex < 0 || p.parameterIndex < 0)
		return;

	ConstantBuffer &cb = ConstantBufferPool[p.bufferIndex];
	ConstantBuffer::Parameter &pCB = cb.parameters[p.parameterIndex];
	uint8 *pointer = cb.data.get() + pCB.offset;
	if (memcmp(pointer, data, pCB.bytes))
	{
		memcpy(pointer, data, pCB.bytes);
		cb.needFlush = true;
	}
}

auto DX11Shader::SetFloatParameter(const char *name, float value) -> void
{
	setParameter(name, &value);
}

auto DX11Shader::SetVec4Parameter(const char *name, const vec4 *value) -> void
{
	setParameter(name, value);
}

auto DX11Shader::SetMat4Parameter(const char *name, const mat4 *value) -> void
{
	setParameter(name, value);
}

auto DX11Shader::SetUintParameter(const char *name, uint value) -> void
{
	setParameter(name, &value);
}

auto DX11Shader::FlushParameters() -> void
{
	ID3D11DeviceContext *ctx = getContext();

	auto updateBuffers = [ctx](vector<size_t>& indicies)
	{
		for (size_t &idx : indicies)
		{
			ConstantBuffer& cb = ConstantBufferPool[idx];
			if (cb.needFlush)
			{
				D3D11_MAPPED_SUBRESOURCE mappedResource{};
				ctx->Map(cb.buffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);

				memcpy(mappedResource.pData, cb.data.get(), cb.bytes);

				ctx->Unmap(cb.buffer.Get(), 0);

				cb.needFlush = false;
			}
		}
	};

	if (vs()) updateBuffers(v.buffers);
	if (fs()) updateBuffers(f.buffers);
	if (gs()) updateBuffers(g.buffers);
	if (cs()) updateBuffers(c.buffers);
}


