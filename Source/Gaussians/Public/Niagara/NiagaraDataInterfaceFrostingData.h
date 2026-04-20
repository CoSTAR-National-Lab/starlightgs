#pragma once

#include "GaussianData.h"
#include "NiagaraDataInterface.h"
#include "GSplatAsset.h"
#include "GaussianData.h"
#include "GaussiansNDICommon.h"
#include "HLSLTypeAliases.h"

#include "RenderGraphBuilder.h"
#include "RenderGraphUtils.h"
#include "UnifiedBuffer.h"
#include "Frosting/FrostingAsset.h"

#include "NiagaraDataInterfaceFrostingData.generated.h"
/**
 * data interface to pass gaussian data to niagara system 
 */

//Shader Parameters

BEGIN_SHADER_PARAMETER_STRUCT(FGFrostingParameters, )
	SHADER_PARAMETER(int, FrostingSplatCount)
	SHADER_PARAMETER_RDG_BUFFER_SRV(StructuredBuffer<FPrismBarycentric>, Barycentrics)
	SHADER_PARAMETER_RDG_BUFFER_SRV(Buffer<int>, FaceIndex)
	SHADER_PARAMETER_RDG_BUFFER_SRV(Buffer<float3>, Scales)
	SHADER_PARAMETER_RDG_BUFFER_SRV(Buffer<float4>, Orientations)
	SHADER_PARAMETER_RDG_BUFFER_SRV(Buffer<float4>, SHAndOpacities)
END_SHADER_PARAMETER_STRUCT()

enum class ENiagaraSimTarget : uint8;
struct FNiagaraDataInterfaceGeneratedFunction;
struct FNiagaraFunctionSignature;
struct FVMExternalFunctionBindingInfo;

/** Render buffers that will be used in hlsl functions */
//struct FNDIGaussianFrostingDataBuffer : public FRenderResource
//{
	//------------------------------------------------------------------------------------------------------------

//	void InitRHI(FRHICommandListBase& RHICmdList) override
//	{
//		if (NumSplats > 0)
//		{
//			FaceIndex.Initialize(RHICmdList, TEXT("FNDIGaussianSplatDataBufferF"), sizeof(int), NumSplats, PF_R32_SINT, BUF_Static);
//			PrismBarycentric.Initialize(RHICmdList, TEXT("FNDIGaussianSplatDataBufferB"), sizeof(FPrismBarycentric), NumSplats, PF_Unknown, BUF_StructuredBuffer);
//			Normals.Initialize(RHICmdList, TEXT("FNDIGaussianSplatDataBufferN"), sizeof(FVector4f), NumSplats, PF_A32B32G32R32F, BUF_Static);
//			Scales.Initialize(RHICmdList, TEXT("FNDIGaussianSplatDataBufferS"), sizeof(FVector4f), NumSplats, PF_A32B32G32R32F, BUF_Static);
//			Orientations.Initialize(RHICmdList, TEXT("FNDIGaussianSplatDataBufferO"), sizeof(FVector4f), NumSplats, PF_A32B32G32R32F, BUF_Static);
//			SHAndOpacities.Initialize(RHICmdList, TEXT("FNDIGaussianSplatDataBufferSHO"), sizeof(FVector4f), NumSplats, PF_A32B32G32R32F, BUF_Static);
//		}
//	}
//
//	void ReleaseRHI() override
//	{
//		FaceIndex.Release();
//		PrismBarycentric.Release();
//		Normals.Release();
//		Scales.Release();
//		Orientations.Release();
//		SHAndOpacities.Release();
//	}
//
//	/** Get the resource name */
//	virtual FString GetFriendlyName() const override { return TEXT("FNDIGaussianSplatDataBuffer"); }
//
//	/** Prism barycentric coords buffer */
//	FReadBuffer PrismBarycentric;
//
//	/** FaceIndex */
//	FReadBuffer FaceIndex;
//
//	/** Inverse transform buffer*/
//	FReadBuffer Normals;
//
//	/** World transform buffer */
//	FReadBuffer Orientations;
//
//	/** Inverse transform buffer*/
//	FReadBuffer Scales;
//
//	/** Element extent buffer */
//	FReadBuffer SHAndOpacities;
//
//	/** number of transforms */
//	uint32 NumSplats = 0;
//
//	void SetNumSplats(uint32 Num)
//	{
//		NumSplats = Num;
//	}
//};

struct FRDGGaussianFrostingData
{
	RDGBufferStruct FaceIndices;
	RDGBufferStruct PrismBarycentric;
	RDGBufferStruct Scales;
	RDGBufferStruct Orientations;
	RDGBufferStruct SHAndOpacities;

	void ResizeBuffers(FRDGBuilder& GraphBuilder, uint32_t nElementCount)
	{
		FRDGBufferDesc BufferDescF3 = FRDGBufferDesc::CreateBufferDesc(sizeof(FVector3f), nElementCount);
		FRDGBufferDesc BufferDescF4 = FRDGBufferDesc::CreateBufferDesc(sizeof(FVector4f), nElementCount);
		FRDGBufferDesc BufferDescF1 = FRDGBufferDesc::CreateBufferDesc(sizeof(float), nElementCount);
		FRDGBufferDesc BufferDescStructured = FRDGBufferDesc::CreateStructuredDesc(sizeof(FPrismBarycentric), nElementCount);

		//FRDGBufferDesc BufferDesc = FRDGBufferDesc::CreateBufferDesc(nElementCount);
		//FRDGBufferDesc BufferDescF4 = FRDGBufferDesc::CreateBufferDesc<FShaderParameters>(nElementCount);

		ResizeBufferIfNeeded(GraphBuilder, PrismBarycentric.PooledBuffer, BufferDescStructured , TEXT("NiagaraFrosting_B"));
		ResizeBufferIfNeeded(GraphBuilder, FaceIndices.PooledBuffer, BufferDescF1, TEXT("NiagaraFrosting_F"));

		ResizeBufferIfNeeded(GraphBuilder, Scales.PooledBuffer, BufferDescF3, TEXT("NiagaraFrosting_S"));
		ResizeBufferIfNeeded(GraphBuilder, Orientations.PooledBuffer, BufferDescF4, TEXT("NiagaraFrosting_O"));
		ResizeBufferIfNeeded(GraphBuilder, SHAndOpacities.PooledBuffer, BufferDescF4, TEXT("NiagaraFrosting_SH"));
	}

	void UploadData(FRDGBuilder& GraphBuilder, FGaussianFrostingData& IntialData)
	{
		//RDG_EVENT_SCOPE(GraphBuilder, "NDIGSBufferDataUpload");

		void* Data = IntialData.BarryCoords.GetData();
		auto DataSize = IntialData.BarryCoords.Num() * IntialData.BarryCoords.GetTypeSize();

		GraphBuilder.QueueBufferUpload(
			GraphBuilder.RegisterExternalBuffer(PrismBarycentric.PooledBuffer),
			Data,
			DataSize,
			[DataBuffer = MoveTemp(IntialData.BarryCoords)](const void*) {}
		);

		Data = IntialData.FaceIndices.GetData();
		DataSize = IntialData.FaceIndices.Num() * IntialData.FaceIndices.GetTypeSize();

		GraphBuilder.QueueBufferUpload(
			GraphBuilder.RegisterExternalBuffer(FaceIndices.PooledBuffer),
			Data,
			DataSize,
			[DataBuffer = MoveTemp(IntialData.FaceIndices)](const void*) {}
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

		Data = IntialData.SH0AndOpacities.GetData();
		DataSize = IntialData.SH0AndOpacities.Num() * IntialData.SH0AndOpacities.GetTypeSize();

		GraphBuilder.QueueBufferUpload(
			GraphBuilder.RegisterExternalBuffer(SHAndOpacities.PooledBuffer),
			Data,
			DataSize,
			[DataBuffer = MoveTemp(IntialData.SH0AndOpacities)](const void*) {}
		);

		IntialData.SplatCount = 0;
	}

	void Register(FRDGBuilder& GraphBuilder)
	{
		if (PrismBarycentric.RDGTransientBuffer == nullptr)
		{
			PrismBarycentric.RDGTransientBuffer = GraphBuilder.RegisterExternalBuffer(PrismBarycentric.PooledBuffer);
			PrismBarycentric.RDGTransientBufferSRV = GraphBuilder.CreateSRV(PrismBarycentric.RDGTransientBuffer);
		}

		if (FaceIndices.RDGTransientBuffer == nullptr)
		{
			FaceIndices.RDGTransientBuffer = GraphBuilder.RegisterExternalBuffer(FaceIndices.PooledBuffer);
			FaceIndices.RDGTransientBufferSRV = GraphBuilder.CreateSRV(FaceIndices.RDGTransientBuffer, PF_R32_SINT);
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
		FaceIndices.RDGTransientBuffer =			nullptr;
		FaceIndices.RDGTransientBufferSRV = 		nullptr;
		PrismBarycentric.RDGTransientBuffer = nullptr;
		PrismBarycentric.RDGTransientBufferSRV = nullptr;

		Scales.RDGTransientBuffer = 			nullptr;
		Scales.RDGTransientBufferSRV = 			nullptr;
		Orientations.RDGTransientBuffer = 		nullptr;
		Orientations.RDGTransientBufferSRV = 	nullptr;
		SHAndOpacities.RDGTransientBuffer = 	nullptr;
		SHAndOpacities.RDGTransientBufferSRV = 	nullptr;
	}

	void Release()
	{
		FaceIndices.Release();
		PrismBarycentric.Release();

		Orientations.Release();
		Scales.Release();
		SHAndOpacities.Release();
	}
};


struct FNDIGaussianFrostingData_GameThread
{
	/** Initialize the cpu datas */
	void Init(class UNiagaraDataInterfaceFrostingData* Interface, FNiagaraSystemInstance* SystemInstance);

	/**asset Cpu arrays */
	FGaussianFrostingData AssetArrays;

	// True if we need to upload new data to the gpu
	bool bUsedWithGpu;
	bool bNeedsGpuSync;
};

struct FGameToRenderData_Frosting
{
	explicit FGameToRenderData_Frosting(FNDIGaussianFrostingData_GameThread& InstanceData)
	{
		if (InstanceData.bNeedsGpuSync)
		{
			
			InitialCpuBufferData.CopyFrom(&InstanceData.AssetArrays);
		}
	}

	explicit FGameToRenderData_Frosting(const FGaussianFrostingData& DataToSend)
	{
		InitialCpuBufferData = DataToSend;
	}

	FGaussianFrostingData	InitialCpuBufferData;
};



struct FNDIGaussianFrostingData_RenderThread
{
	//FNDIGaussianSplatDataBuffer AssetBuffer;

	uint32 NumElements;
	FRDGGaussianFrostingData AssetBuffer;
	FGaussianFrostingData InitialCpuBufferData;
	

	~FNDIGaussianFrostingData_RenderThread()
	{
		AssetBuffer.Release();
	}

	void UpdateData(FGameToRenderData_Frosting& GameData)
	{
		AssetBuffer.Release();

		NumElements = GameData.InitialCpuBufferData.GetSplatCount();
		InitialCpuBufferData = MoveTemp(GameData.InitialCpuBufferData);
	}
};



UCLASS(EditInlineNew, Category = "Gaussians", meta = (DisplayName = "Frosting Splat Data Query"))
class GAUSSIANS_API UNiagaraDataInterfaceFrostingData : public UNiagaraDataInterface
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, Category = "GSplat Collection")
	TObjectPtr<UFrostingDataAsset> FrostingDataSource;

	UNiagaraDataInterfaceFrostingData(FObjectInitializer const& ObjectInitializer);
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

	virtual int32 PerInstanceDataSize() const override { return sizeof(FNDIGaussianFrostingData_GameThread); };

	virtual bool HasPreSimulateTick() const override { return true; }

	virtual bool PerInstanceTick(void* PerInstanceData, FNiagaraSystemInstance* SystemInstance, float DeltaSeconds) override;

	
	//virtual void ProvidePerInstanceDataForRenderThread(void* DataForRenderThread, void* PerInstanceData, const FNiagaraSystemInstanceID& SystemInstance) override;


	void GetFrostingDataVM(FVectorVMExternalFunctionContext& Context);
	void GetSplatCountVM(FVectorVMExternalFunctionContext& Context);

private:
	inline static const FName GetSplatDataFuncName  = FName(TEXT("GetFrostingData"));
	inline static const FName GetFrostingBarycentricFuncName  = FName(TEXT("GetFrostingBarycentricData"));
	inline static const FName GetSplatCountFuncName = FName(TEXT("GetFrostingSplatCount"));

	FGFrostingParameters Params;
};

// this proxy is used to safely copy data between game thread and render thread
struct FNDIGaussianFrostingProxy : public FNiagaraDataInterfaceProxy
{
	virtual int32 PerInstanceDataPassedToRenderThreadSize() const override { return 0; }

	/** Get the data that will be passed to render*/
	virtual void ConsumePerInstanceDataFromGameThread(void* PerInstanceData, const FNiagaraSystemInstanceID& Instance) override;

	/** Launch all pre stage functions */
	virtual void PreStage(const FNDIGpuComputePreStageContext& Context) override;

	virtual void PostSimulate(const FNDIGpuComputePostSimulateContext& Context) override;

	/** List of proxy data for each system instances*/
	TMap<FNiagaraSystemInstanceID, FNDIGaussianFrostingData_RenderThread>	PerInstanceData_RenderThread;
	TMap<FNiagaraSystemInstanceID, FNDIGaussianFrostingData_GameThread*>	PerInstanceData_GameThread;
};