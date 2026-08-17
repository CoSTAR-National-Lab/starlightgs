#include "Niagara/NiagaraDataInterfaceGaussianData.h"

#if WITH_EDITORONLY_DATA
#include "LevelEditorViewport.h"
#else
#include "Engine/World.h"
#endif

#include "GaussianData.h"
#include "NiagaraCompileHashVisitor.h"
#include "NiagaraShaderParametersBuilder.h"
#include "NiagaraSystemInstance.h"
#include "NiagaraSimStageData.h"
#include "RenderGraphBuilder.h"
#include "RenderGraphUtils.h"
#include "UnifiedBuffer.h"


#define LOCTEXT_NAMESPACE "NiagaraDataInterfaceGaussianData"
static const TCHAR* GSplatDITemplateShaderFile = TEXT("/Plugin/StarLightGS/GSplatNiagaraDataInterface.ush");


//------------------------------------------------------------------------------------------------------------

void FNDIGaussianSplatData_GameThread::Init(UNiagaraDataInterfaceGaussianData* Interface, FNiagaraSystemInstance* SystemInstance)
{
    if (Interface && SystemInstance)
    {
        bNeedsGpuSync = (Interface->IsUsedWithGPUScript()) ? true : false;

		if(Interface->SplatDataSource.Get())
		{
			AssetArrays.CopyFrom(&Interface->SplatDataSource.Get()->Splats);
            AssetArrays.SplatCount = AssetArrays.Positions.Num();
		}
		else
		{
			AssetArrays.Resize(1);
		}
    }
}

//------------------------------------------------------------------------------------------------------------


UNiagaraDataInterfaceGaussianData::UNiagaraDataInterfaceGaussianData(FObjectInitializer const& ObjectInitializer)
{
    Proxy.Reset(new FNDIGaussianSplatProxy);
}

void UNiagaraDataInterfaceGaussianData::PostInitProperties()
{
	Super::PostInitProperties();

    if (HasAnyFlags(RF_ClassDefaultObject))
    {
        ENiagaraTypeRegistryFlags Flags = ENiagaraTypeRegistryFlags::AllowAnyVariable | ENiagaraTypeRegistryFlags::AllowParameter;
        FNiagaraTypeRegistry::Register(FNiagaraTypeDefinition(GetClass()), Flags);
    }
}

void UNiagaraDataInterfaceGaussianData::GetFunctions(TArray<FNiagaraFunctionSignature>& OutFunctions)
{
    {
        FNiagaraFunctionSignature Sig;
        Sig.bSupportsCPU = true;
        Sig.bSupportsGPU = true;
        Sig.bReadFunction = true;
        Sig.bRequiresExecPin = false;
        Sig.Name = TEXT("GetSplatCount");
        Sig.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition(GetClass()),
            TEXT("GaussianSplatNDI")));
        Sig.Outputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetIntDef(),
            TEXT("Count")));
        Sig.bMemberFunction = true;
        Sig.bRequiresContext = false;
        NIAGARA_ADD_FUNCTION_SOURCE_INFO(Sig)
        OutFunctions.Add(Sig);
    }

	{
	    FNiagaraFunctionSignature Sig;
	    Sig.bSupportsCPU = true;
	    Sig.bSupportsGPU = true;
	    Sig.bReadFunction = true;
	    Sig.bRequiresExecPin = false;
	    Sig.Name = TEXT("GetSplatData");
	    Sig.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition(GetClass()),
	        TEXT("GaussianSplatNDI")));
	    Sig.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetIntDef(),
	        TEXT("Index")));
	    Sig.Outputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetVec3Def(),
	        TEXT("Position")));
	   /* Sig.Outputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetVec3Def(),
	        TEXT("Normal")));*/
	    Sig.Outputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetVec3Def(),
	        TEXT("Scale")));
	    Sig.Outputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetQuatDef(),
	        TEXT("Orientation")));
	    Sig.Outputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetVec4Def(),
	        TEXT("SHAndOpacity")));
	    Sig.bMemberFunction = true;
	    Sig.bRequiresContext = false;
	    NIAGARA_ADD_FUNCTION_SOURCE_INFO(Sig)
	    OutFunctions.Add(Sig);
    }
};


void UNiagaraDataInterfaceGaussianData::GetVMExternalFunction(const FVMExternalFunctionBindingInfo& BindingInfo, void* InstanceData, FVMExternalFunction& OutFunc)
{
    if (BindingInfo.Name == GetSplatDataFuncName)
    {
        OutFunc = FVMExternalFunction::CreateLambda([this](FVectorVMExternalFunctionContext& Context) { this->GetSplatDataVM(Context); });
    }

    if (BindingInfo.Name == GetSplatCountFuncName)
    {
        OutFunc = FVMExternalFunction::CreateLambda([this](FVectorVMExternalFunctionContext& Context) { this->GetSplatCountVM(Context); });
    }
}

#ifdef WITH_EDITORONLY_DATA
bool UNiagaraDataInterfaceGaussianData::AppendCompileHash(FNiagaraCompileHashVisitor* InVisitor) const
{
    if (!Super::AppendCompileHash(InVisitor))
    {
        return false;
    }
    InVisitor->UpdateShaderFile(GSplatDITemplateShaderFile);
    InVisitor->UpdateShaderParameters<FGSplatParameters>();
    return true;
}

bool UNiagaraDataInterfaceGaussianData::GetFunctionHLSL(const FNiagaraDataInterfaceGPUParamInfo& ParamInfo, const FNiagaraDataInterfaceGeneratedFunction& FunctionInfo, int FunctionInstanceIndex, FString& OutHLSL)
{
    return FunctionInfo.DefinitionName == GetSplatDataFuncName || FunctionInfo.DefinitionName == GetSplatCountFuncName;
}

void UNiagaraDataInterfaceGaussianData::GetParameterDefinitionHLSL(const FNiagaraDataInterfaceGPUParamInfo& ParamInfo, FString& OutHLSL)
{
    const TMap<FString, FStringFormatArg> TemplateArgs =
    {
        {TEXT("ParameterName"),	ParamInfo.DataInterfaceHLSLSymbol},
    };
    AppendTemplateHLSL(OutHLSL, GSplatDITemplateShaderFile, TemplateArgs);
}
#endif

bool UNiagaraDataInterfaceGaussianData::Equals(const UNiagaraDataInterface* Other) const
{
    if (!Super::Equals(Other))
    {
        return false;
    }
    const UNiagaraDataInterfaceGaussianData* OtherTyped = CastChecked<const UNiagaraDataInterfaceGaussianData>(Other);

    return  OtherTyped->SplatDataSource == SplatDataSource;
}

void UNiagaraDataInterfaceGaussianData::BuildShaderParameters(FNiagaraShaderParametersBuilder& ShaderParametersBuilder) const
{
    ShaderParametersBuilder.AddNestedStruct<FGSplatParameters>();
}

void UNiagaraDataInterfaceGaussianData::SetShaderParameters(const FNiagaraDataInterfaceSetShaderParametersContext& Context) const
{
    // Initializing the shader parameters to be the same reference of 
  //our buffers in the proxy
    FGSplatParameters* ShaderParameters =
        Context.GetParameterNestedStruct<FGSplatParameters>();
    if (ShaderParameters)
    {
        FRDGBuilder& GraphBuilder = Context.GetGraphBuilder();
        FNDIGaussianSplatProxy& DataInterfaceProxy = Context.GetProxy<FNDIGaussianSplatProxy>();
        auto& InstanceData = DataInterfaceProxy.PerInstanceData_RenderThread.FindChecked(Context.GetSystemInstanceID());

        // Constants
        ShaderParameters->SplatCount = InstanceData.NumElements;
        /// Assign initialized buffers to shader parameters

        InstanceData.AssetBuffer.Register(GraphBuilder);

        ShaderParameters->Positions = InstanceData.AssetBuffer.Positions.RDGTransientBufferSRV;
        ShaderParameters->Scales = InstanceData.AssetBuffer.Scales.RDGTransientBufferSRV;
        ShaderParameters->Orientations = InstanceData.AssetBuffer.Orientations.RDGTransientBufferSRV;
        //ShaderParameters->Normals = InstanceData.AssetBuffer.Normals.SRV;
        ShaderParameters->SHAndOpacities = InstanceData.AssetBuffer.SHAndOpacities.RDGTransientBufferSRV;
    }
}

bool UNiagaraDataInterfaceGaussianData::CopyToInternal(UNiagaraDataInterface* Destination) const
{
    if (!Super::CopyToInternal(Destination))
    {
        return false;
    }

    UNiagaraDataInterfaceGaussianData* OtherTyped = CastChecked<UNiagaraDataInterfaceGaussianData>(Destination);

    OtherTyped->SplatDataSource = SplatDataSource;
    return true;
}

bool UNiagaraDataInterfaceGaussianData::InitPerInstanceData(void* PerInstanceData,
                                                            FNiagaraSystemInstance* SystemInstance)
{
    FNDIGaussianSplatData_GameThread* InstanceData = new (PerInstanceData) FNDIGaussianSplatData_GameThread();

    check(InstanceData);
    InstanceData->Init(this, SystemInstance);

    if (IsUsedWithGPUScript())
    {
        InstanceData->bUsedWithGpu = true;

        FNDIGaussianSplatProxy* Proxy_GT = GetProxyAs<FNDIGaussianSplatProxy>();
        Proxy_GT->PerInstanceData_GameThread.Add(SystemInstance->GetId(), InstanceData);
    }


    return true;
}

void UNiagaraDataInterfaceGaussianData::DestroyPerInstanceData(void* PerInstanceData,
	FNiagaraSystemInstance* SystemInstance)
{
    FNDIGaussianSplatData_GameThread* InstanceData = static_cast<FNDIGaussianSplatData_GameThread*>(PerInstanceData);

    if (InstanceData->bUsedWithGpu)
    {
        FNDIGaussianSplatProxy* Proxy_GT = GetProxyAs<FNDIGaussianSplatProxy>();
        Proxy_GT->PerInstanceData_GameThread.Remove(SystemInstance->GetId());

        ENQUEUE_RENDER_COMMAND(FNDIMemoryBuffer_RemoveProxy)
            (
                [RT_Proxy = Proxy_GT, InstanceID = SystemInstance->GetId()](FRHICommandListImmediate& CmdList)
                {
                    RT_Proxy->PerInstanceData_RenderThread.Remove(InstanceID);
                }
                );
    }

    InstanceData->~FNDIGaussianSplatData_GameThread();
}


bool UNiagaraDataInterfaceGaussianData::PerInstanceTick(void* PerInstanceData, FNiagaraSystemInstance* SystemInstance,
	float DeltaSeconds)
{
    FNDIGaussianSplatData_GameThread* InstanceData = reinterpret_cast<FNDIGaussianSplatData_GameThread*>(PerInstanceData);

    if (InstanceData->bUsedWithGpu && InstanceData->bNeedsGpuSync)
    {
        ENQUEUE_RENDER_COMMAND(FNDIMemoryBuffer_UpdateData)
            (
                [RT_Proxy = GetProxyAs<FNDIGaussianSplatProxy>(), RT_SystemInstanceID = SystemInstance->GetId(), RT_GameToRenderData = FGameToRenderData_Splats(*InstanceData)](FRHICommandListImmediate& RHICmdList) mutable
                {
                    FNDIGaussianSplatData_RenderThread& InstanceData_RT = RT_Proxy->PerInstanceData_RenderThread.FindOrAdd(RT_SystemInstanceID);
                    InstanceData_RT.UpdateData(RT_GameToRenderData);
                }
                );

        InstanceData->bNeedsGpuSync = false;
    }

    return false;
}

void UNiagaraDataInterfaceGaussianData::GetSplatDataVM(FVectorVMExternalFunctionContext& Context)
{
    // Input is the NDI and Index of the particle
    VectorVM::FUserPtrHandler<FNDIGaussianSplatData_GameThread>
        InstData(Context);

    FNDIInputParam<int32> IndexParam(Context);

    // Output Position
    FNDIOutputParam<FVector3f> Positions(Context);
    //FNDIOutputParam<FVector3f> Normals(Context);
    FNDIOutputParam<FVector3f> Scales(Context);
    FNDIOutputParam<FQuat4f> Orientations(Context);
    FNDIOutputParam<FVector4f> SHAndOpacities(Context);

    auto& data = InstData->AssetArrays;

    const auto InstancesCount = Context.GetNumInstances();

    for (int32 i = 0; i < InstancesCount; ++i)
    {
        const int32 Index = IndexParam.GetAndAdvance();

        if (data.Positions.IsValidIndex(Index))
        {
            Positions.SetAndAdvance(data.Positions[Index]);
            //Normals.SetAndAdvance(data.Normals[Index]);
            Scales.SetAndAdvance(data.Scales[Index]);
            Orientations.SetAndAdvance(data.Orientations[Index]);
        	SHAndOpacities.SetAndAdvance(data.ZeroOrderHarmonicsCoefficientsAndOpacities[Index]);
        }
        else
        {
            Positions.SetAndAdvance(FVector3f::ZeroVector);
            //Normals.SetAndAdvance(FVector3f::ZeroVector);
            Scales.SetAndAdvance(FVector3f::ZeroVector);
            Orientations.SetAndAdvance(FQuat4f::Identity);
            SHAndOpacities.SetAndAdvance(FVector4f::Zero());
        }
    }
}

void UNiagaraDataInterfaceGaussianData::GetSplatCountVM(FVectorVMExternalFunctionContext& Context)
{
    // Input is the NDI and Index of the particle
    VectorVM::FUserPtrHandler<FNDIGaussianSplatData_GameThread>
        InstData(Context);

    // Output Position
    FNDIOutputParam<int> OutCount(Context);

    const auto InstancesCount = Context.GetNumInstances();

    for (int32 i = 0; i < InstancesCount; ++i)
    {
        OutCount.SetAndAdvance(InstData.Get()->AssetArrays.SplatCount);
    }
}

void FNDIGaussianSplatProxy::ConsumePerInstanceDataFromGameThread(void* PerInstanceData,
	const FNiagaraSystemInstanceID& Instance)
{
    checkNoEntry();
}

void FNDIGaussianSplatProxy::PreStage(const FNDIGpuComputePreStageContext& Context)
{
    const FNiagaraSystemInstanceID SystemInstanceID = Context.GetSystemInstanceID();
    FNDIGaussianSplatData_RenderThread& InstanceData = PerInstanceData_RenderThread.FindChecked(SystemInstanceID);
    const FNiagaraSimStageData& SimStageData = Context.GetSimStageData();

    if (SimStageData.bFirstStage)
    {
        FRDGBuilder& GraphBuilder = Context.GetGraphBuilder();

        if (InstanceData.InitialCpuBufferData.SplatCount > 0)
        {
            InstanceData.AssetBuffer.ResizeBuffers(GraphBuilder, InstanceData.InitialCpuBufferData.SplatCount);
            InstanceData.AssetBuffer.UploadData(GraphBuilder, InstanceData.InitialCpuBufferData);
        }
        
    }
}

void FNDIGaussianSplatProxy::PostSimulate(const FNDIGpuComputePostSimulateContext& Context)
{
	if (!Context.IsFinalPostSimulate())
	{
		return;
	}

	const FNiagaraSystemInstanceID SystemInstanceID = Context.GetSystemInstanceID();
	FNDIGaussianSplatData_RenderThread& InstanceData = PerInstanceData_RenderThread.FindChecked(SystemInstanceID);

	// Clear out transient pointers we cache for graph usage
    InstanceData.AssetBuffer.ClearTransient();
}

#undef LOCTEXT_NAMESPACE