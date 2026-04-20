#pragma once

#include "GaussianData.h"
#include "NiagaraDataInterface.h"
#include "GSplatAsset.h"
#include "GaussianData.h"
#include "GaussiansNDICommon.h"
#include "HLSLTypeAliases.h"



#include "NiagaraDataInterfaceGaussianData.generated.h"
/**
 * data interface to pass gaussian data to niagara system 
 */

//Shader Parameters

BEGIN_SHADER_PARAMETER_STRUCT(FGSplatParameters, )
	SHADER_PARAMETER(int, SplatCount)
	SHADER_PARAMETER_RDG_BUFFER_SRV(Buffer<float3>, Positions)
	//SHADER_PARAMETER_SRV(Buffer<float4>, Normals)
	SHADER_PARAMETER_RDG_BUFFER_SRV(Buffer<float3>, Scales)
	SHADER_PARAMETER_RDG_BUFFER_SRV(Buffer<float4>, Orientations)
	SHADER_PARAMETER_RDG_BUFFER_SRV(Buffer<float4>, SHAndOpacities)
END_SHADER_PARAMETER_STRUCT()

enum class ENiagaraSimTarget : uint8;
struct FNiagaraDataInterfaceGeneratedFunction;
struct FNiagaraFunctionSignature;
struct FVMExternalFunctionBindingInfo;

/** Render buffers that will be used in hlsl functions */
struct FNDIGaussianSplatDataBuffer : public FRenderResource
{
	//------------------------------------------------------------------------------------------------------------

	void InitRHI(FRHICommandListBase& RHICmdList) override
	{
		if (NumSplats > 0)
		{
			Positions.Initialize(RHICmdList, TEXT("FNDIGaussianSplatDataBufferP"), sizeof(FVector4f), NumSplats, PF_A32B32G32R32F, BUF_Static);
			Normals.Initialize(RHICmdList, TEXT("FNDIGaussianSplatDataBufferN"), sizeof(FVector4f), NumSplats, PF_A32B32G32R32F, BUF_Static);
			Scales.Initialize(RHICmdList, TEXT("FNDIGaussianSplatDataBufferS"), sizeof(FVector4f), NumSplats, PF_A32B32G32R32F, BUF_Static);
			Orientations.Initialize(RHICmdList, TEXT("FNDIGaussianSplatDataBufferO"), sizeof(FVector4f), NumSplats, PF_A32B32G32R32F, BUF_Static);
			SHAndOpacities.Initialize(RHICmdList, TEXT("FNDIGaussianSplatDataBufferSHO"), sizeof(FVector4f), NumSplats, PF_A32B32G32R32F, BUF_Static);
		}
	}

	void ReleaseRHI() override
	{
		Positions.Release();
		Normals.Release();
		Scales.Release();
		Orientations.Release();
		SHAndOpacities.Release();
	}

	/** Get the resource name */
	virtual FString GetFriendlyName() const override { return TEXT("FNDIGaussianSplatDataBuffer"); }

	/** World transform buffer */
	FReadBuffer Positions;

	/** Inverse transform buffer*/
	FReadBuffer Normals;

	/** World transform buffer */
	FReadBuffer Orientations;

	/** Inverse transform buffer*/
	FReadBuffer Scales;

	/** Element extent buffer */
	FReadBuffer SHAndOpacities;

	/** number of transforms */
	uint32 NumSplats = 0;

	void SetNumSplats(uint32 Num)
	{
		NumSplats = Num;
	}
};


struct FRDGGaussianSplatData
{
	RDGBufferStruct Positions;
	RDGBufferStruct Scales;
	RDGBufferStruct Orientations;
	RDGBufferStruct SHAndOpacities;

	void ResizeBuffers(FRDGBuilder& GraphBuilder, uint32_t nElementCount)
	{
		FRDGBufferDesc BufferDescF3 = FRDGBufferDesc::CreateBufferDesc(sizeof(FVector3f), nElementCount);
		FRDGBufferDesc BufferDescF4 = FRDGBufferDesc::CreateBufferDesc(sizeof(FVector4f), nElementCount);

		//FRDGBufferDesc BufferDesc = FRDGBufferDesc::CreateBufferDesc(nElementCount);
		//FRDGBufferDesc BufferDescF4 = FRDGBufferDesc::CreateBufferDesc<FShaderParameters>(nElementCount);

		ResizeBufferIfNeeded(GraphBuilder, Positions.PooledBuffer, BufferDescF3, TEXT("NiagaraGS_P"));
		ResizeBufferIfNeeded(GraphBuilder, Scales.PooledBuffer, BufferDescF3, TEXT("NiagaraGS_S"));
		ResizeBufferIfNeeded(GraphBuilder, Orientations.PooledBuffer, BufferDescF4, TEXT("NiagaraGS_O"));
		ResizeBufferIfNeeded(GraphBuilder, SHAndOpacities.PooledBuffer, BufferDescF4, TEXT("NiagaraGS_SH"));
	}

	void UploadData(FRDGBuilder& GraphBuilder, FGaussianSplatData& IntialData)
	{
		//RDG_EVENT_SCOPE(GraphBuilder, "NDIGSBufferDataUpload");

		void* Data = IntialData.Positions.GetData();
		auto DataSize = IntialData.Positions.Num() * IntialData.Positions.GetTypeSize();

		GraphBuilder.QueueBufferUpload(
			GraphBuilder.RegisterExternalBuffer(Positions.PooledBuffer),
			Data,
			DataSize,
			[DataBuffer = MoveTemp(IntialData.Positions)](const void*) {}
		);

		Data = IntialData.Scales.GetData();
		DataSize = IntialData.Scales.Num() * IntialData.Scales.GetTypeSize();

		GraphBuilder.QueueBufferUpload(
			GraphBuilder.RegisterExternalBuffer(Scales.PooledBuffer),
			Data,
			DataSize,
			[DataBuffer = MoveTemp(IntialData.Scales)](const void*) {}
		);

		Data = IntialData.Orientations.GetData();
		DataSize = IntialData.Orientations.Num() * IntialData.Orientations.GetTypeSize();

		GraphBuilder.QueueBufferUpload(
			GraphBuilder.RegisterExternalBuffer(Orientations.PooledBuffer),
			Data,
			DataSize,
			[DataBuffer = MoveTemp(IntialData.Orientations)](const void*) {}
		);

		Data = IntialData.ZeroOrderHarmonicsCoefficientsAndOpacities.GetData();
		DataSize = IntialData.ZeroOrderHarmonicsCoefficientsAndOpacities.Num() * IntialData.ZeroOrderHarmonicsCoefficientsAndOpacities.GetTypeSize();

		GraphBuilder.QueueBufferUpload(
			GraphBuilder.RegisterExternalBuffer(SHAndOpacities.PooledBuffer),
			Data,
			DataSize,
			[DataBuffer = MoveTemp(IntialData.ZeroOrderHarmonicsCoefficientsAndOpacities)](const void*) {}
		);

		IntialData.SplatCount = 0;
	}

	void Register(FRDGBuilder& GraphBuilder)
	{
		if (Positions.RDGTransientBuffer == nullptr)
		{
			Positions.RDGTransientBuffer = GraphBuilder.RegisterExternalBuffer(Positions.PooledBuffer);
			Positions.RDGTransientBufferSRV = GraphBuilder.CreateSRV(Positions.RDGTransientBuffer,PF_R32G32B32F);
		}

		if (Scales.RDGTransientBuffer == nullptr)
		{
			Scales.RDGTransientBuffer = GraphBuilder.RegisterExternalBuffer(Scales.PooledBuffer);
			Scales.RDGTransientBufferSRV = GraphBuilder.CreateSRV(Scales.RDGTransientBuffer, PF_R32G32B32F);
		}

		if (Orientations.RDGTransientBuffer == nullptr)
		{
			Orientations.RDGTransientBuffer = GraphBuilder.RegisterExternalBuffer(Orientations.PooledBuffer);
			Orientations.RDGTransientBufferSRV = GraphBuilder.CreateSRV(Orientations.RDGTransientBuffer, PF_A32B32G32R32F);
		}

		if (SHAndOpacities.RDGTransientBuffer == nullptr)
		{
			SHAndOpacities.RDGTransientBuffer = GraphBuilder.RegisterExternalBuffer(SHAndOpacities.PooledBuffer);
			SHAndOpacities.RDGTransientBufferSRV = GraphBuilder.CreateSRV(SHAndOpacities.RDGTransientBuffer, PF_A32B32G32R32F);
		}
	}

	void ClearTransient()
	{
		Positions.RDGTransientBuffer =			nullptr;
		Positions.RDGTransientBufferSRV = 		nullptr;
		Scales.RDGTransientBuffer = 			nullptr;
		Scales.RDGTransientBufferSRV = 			nullptr;
		Orientations.RDGTransientBuffer = 		nullptr;
		Orientations.RDGTransientBufferSRV = 	nullptr;
		SHAndOpacities.RDGTransientBuffer = 	nullptr;
		SHAndOpacities.RDGTransientBufferSRV = 	nullptr;
	}

	void Release()
	{
		Positions.Release();
		Orientations.Release();
		Scales.Release();
		SHAndOpacities.Release();
	}
};


struct FNDIGaussianSplatData_GameThread
{
	/** Initialize the cpu datas */
	void Init(class UNiagaraDataInterfaceGaussianData* Interface, FNiagaraSystemInstance* SystemInstance);

	/**asset Cpu arrays */
	FGaussianSplatData AssetArrays;

	// True if we need to upload new data to the gpu
	bool bUsedWithGpu;
	bool bNeedsGpuSync;
};

struct FGameToRenderData_Splats
{
	explicit FGameToRenderData_Splats(FNDIGaussianSplatData_GameThread& InstanceData)
	{
		if (InstanceData.bNeedsGpuSync)
		{
			
			InitialCpuBufferData.CopyFrom(&InstanceData.AssetArrays);
		}
	}

	explicit FGameToRenderData_Splats(const FGaussianSplatData& DataToSend)
	{
		InitialCpuBufferData = DataToSend;
	}

	FGaussianSplatData	InitialCpuBufferData;
};



struct FNDIGaussianSplatData_RenderThread
{
	//FNDIGaussianSplatDataBuffer AssetBuffer;

	uint32 NumElements;
	FRDGGaussianSplatData AssetBuffer;
	FGaussianSplatData InitialCpuBufferData;
	

	~FNDIGaussianSplatData_RenderThread()
	{
		AssetBuffer.Release();
	}

	void UpdateData(FGameToRenderData_Splats& GameData)
	{
		AssetBuffer.Release();

		NumElements = GameData.InitialCpuBufferData.SplatCount;
		InitialCpuBufferData = MoveTemp(GameData.InitialCpuBufferData);
	}
};



UCLASS(EditInlineNew, Category = "Gaussian", meta = (DisplayName = "Gaussian Splat Data Query"))
class GAUSSIANS_API UNiagaraDataInterfaceGaussianData : public UNiagaraDataInterface
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, Category = "GSplat Collection")
	TObjectPtr<UGSplatAsset> SplatDataSource;

	UNiagaraDataInterfaceGaussianData(FObjectInitializer const& ObjectInitializer);
	virtual void PostInitProperties() override;

	virtual void GetFunctions(TArray<FNiagaraFunctionSignature>& OutFunctions) override;

	//UNiagaraDataInterface Interface
	virtual void GetVMExternalFunction(const FVMExternalFunctionBindingInfo& BindingInfo, void* InstanceData, FVMExternalFunction& OutFunc) override;
	virtual bool CanExecuteOnTarget(ENiagaraSimTarget Target) const override { return true; }
#if WITH_EDITORONLY_DATA
	virtual bool AppendCompileHash(FNiagaraCompileHashVisitor* InVisitor) const override;
	virtual bool GetFunctionHLSL(const FNiagaraDataInterfaceGPUParamInfo& ParamInfo, const FNiagaraDataInterfaceGeneratedFunction& FunctionInfo, int FunctionInstanceIndex, FString& OutHLSL) override;
	virtual void GetParameterDefinitionHLSL(const FNiagaraDataInterfaceGPUParamInfo& ParamInfo, FString& OutHLSL) override;
#endif
	virtual void BuildShaderParameters(FNiagaraShaderParametersBuilder& ShaderParametersBuilder) const override;
	virtual void SetShaderParameters(const FNiagaraDataInterfaceSetShaderParametersContext& Context) const override;

	/** Copy one niagara DI to this */
	virtual bool CopyToInternal(UNiagaraDataInterface* Destination) const override;

	virtual bool Equals(const UNiagaraDataInterface* Other) const override;

	virtual bool InitPerInstanceData(void* PerInstanceData, FNiagaraSystemInstance* SystemInstance) override;
	virtual void DestroyPerInstanceData(void* PerInstanceData, FNiagaraSystemInstance* SystemInstance) override;

	virtual int32 PerInstanceDataSize() const override { return sizeof(FNDIGaussianSplatData_GameThread); };

	virtual bool HasPreSimulateTick() const override { return true; }

	virtual bool PerInstanceTick(void* PerInstanceData, FNiagaraSystemInstance* SystemInstance, float DeltaSeconds) override;

	
	//virtual void ProvidePerInstanceDataForRenderThread(void* DataForRenderThread, void* PerInstanceData, const FNiagaraSystemInstanceID& SystemInstance) override;


	void GetSplatDataVM(FVectorVMExternalFunctionContext& Context);
	void GetSplatCountVM(FVectorVMExternalFunctionContext& Context);

private:
	inline static const FName GetSplatDataFuncName  = FName(TEXT("GetSplatData"));
	inline static const FName GetSplatCountFuncName = FName(TEXT("GetSplatCount"));

	FGSplatParameters Params;
};

// this proxy is used to safely copy data between game thread and render thread
struct FNDIGaussianSplatProxy : public FNiagaraDataInterfaceProxy
{
	virtual int32 PerInstanceDataPassedToRenderThreadSize() const override { return 0; }

	/** Get the data that will be passed to render*/
	virtual void ConsumePerInstanceDataFromGameThread(void* PerInstanceData, const FNiagaraSystemInstanceID& Instance) override;

	/** Launch all pre stage functions */
	virtual void PreStage(const FNDIGpuComputePreStageContext& Context) override;

	virtual void PostSimulate(const FNDIGpuComputePostSimulateContext& Context) override;

	/** List of proxy data for each system instances*/
	TMap<FNiagaraSystemInstanceID, FNDIGaussianSplatData_RenderThread>	PerInstanceData_RenderThread;
	TMap<FNiagaraSystemInstanceID, FNDIGaussianSplatData_GameThread*>	PerInstanceData_GameThread;
};