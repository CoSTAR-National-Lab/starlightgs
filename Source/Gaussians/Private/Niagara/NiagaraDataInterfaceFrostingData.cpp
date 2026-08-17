#include "Niagara/NiagaraDataInterfaceFrostingData.h"

#if WITH_EDITORONLY_DATA
#include "LevelEditorViewport.h"
#else
#include "Engine/World.h"
#endif

#include "Frosting/FrostingAsset.h"

#include "NiagaraCompileHashVisitor.h"
#include "NiagaraShaderParametersBuilder.h"
#include "NiagaraSystemInstance.h"
#include "NiagaraSimStageData.h"


#define LOCTEXT_NAMESPACE "NiagaraDataInterfaceFrostingData"
static const TCHAR* GFrostingDITemplateShaderFile = TEXT("/Plugin/StarLightGS/GSplatFrostingNiagaraDataInterface.ush");


//------------------------------------------------------------------------------------------------------------

void FNDIGaussianFrostingData_GameThread::Init(UNiagaraDataInterfaceFrostingData* Interface, FNiagaraSystemInstance* SystemInstance)
{
    if (Interface && SystemInstance)
    {
        bNeedsGpuSync = (Interface->IsUsedWithGPUScript()) ? true : false;

		if(Interface->FrostingDataSource.Get())
		{
			AssetArrays.CopyFrom(&Interface->FrostingDataSource.Get()->Frosting.Splats);
            AssetArrays.SplatCount = AssetArrays.BarryCoords.Num();
		}
		else
		{
			AssetArrays.Resize(1);
		}
    }
}

//------------------------------------------------------------------------------------------------------------


UNiagaraDataInterfaceFrostingData::UNiagaraDataInterfaceFrostingData(FObjectInitializer const& ObjectInitializer)
{
    Proxy.Reset(new FNDIGaussianFrostingProxy);
}

void UNiagaraDataInterfaceFrostingData::PostInitProperties()
{
	Super::PostInitProperties();

    if (HasAnyFlags(RF_ClassDefaultObject))
    {
        ENiagaraTypeRegistryFlags Flags = ENiagaraTypeRegistryFlags::AllowAnyVariable | ENiagaraTypeRegistryFlags::AllowParameter;
        FNiagaraTypeRegistry::Register(FNiagaraTypeDefinition(GetClass()), Flags);

        FNiagaraTypeRegistry::Register(FNiagaraTypeDefinition(FPrismBarycentric::StaticStruct()), Flags);
    }
}

void UNiagaraDataInterfaceFrostingData::GetFunctions(TArray<FNiagaraFunctionSignature>& OutFunctions)
{
    {
        FNiagaraFunctionSignature Sig;
        Sig.bSupportsCPU = true;
        Sig.bSupportsGPU = true;
        Sig.bReadFunction = true;
        Sig.bRequiresExecPin = false;
        Sig.Name = GetSplatCountFuncName;
        Sig.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition(GetClass()),
            TEXT("GaussianFrostingNDI")));
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
        Sig.Name = GetSplatDataFuncName;
        Sig.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition(GetClass()),
            TEXT("GaussianFrostingNDI")));
        Sig.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetIntDef(),
            TEXT("Index")));
       /* Sig.Outputs.Add(FNiagaraVariable(FNiagaraTypeDefinition(FPrismBarycentric::StaticStruct()),
            TEXT("Barycentric")));*/
		/* Sig.Outputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetVec3Def(),
            TEXT("Normal")));*/
        Sig.Outputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetIntDef(),
            TEXT("FaceIndex")));

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

    {
        FNiagaraFunctionSignature Sig;
        Sig.bSupportsCPU = true;
        Sig.bSupportsGPU = true;
        Sig.bReadFunction = true;
        Sig.bRequiresExecPin = false;
        Sig.Name = GetFrostingBarycentricFuncName;
        Sig.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition(GetClass()),
            TEXT("GaussianFrostingNDI")));
        Sig.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetIntDef(),
            TEXT("Index")));
        Sig.Outputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetFloatDef(),
            TEXT("a0")));
        Sig.Outputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetFloatDef(),
            TEXT("a1")));
        Sig.Outputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetFloatDef(),
            TEXT("a2")));
        Sig.Outputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetFloatDef(),
            TEXT("a3")));
        Sig.Outputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetFloatDef(),
            TEXT("a4")));
        Sig.Outputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetFloatDef(),
            TEXT("a5")));
        /* Sig.Outputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetVec3Def(),
            TEXT("Normal")));*/

        Sig.bMemberFunction = true;
        Sig.bRequiresContext = false;
        NIAGARA_ADD_FUNCTION_SOURCE_INFO(Sig)
            OutFunctions.Add(Sig);
    }
};


void UNiagaraDataInterfaceFrostingData::GetVMExternalFunction(const FVMExternalFunctionBindingInfo& BindingInfo, void* InstanceData, FVMExternalFunction& OutFunc)
{
    if (BindingInfo.Name == GetSplatDataFuncName)
    {
        OutFunc = FVMExternalFunction::CreateLambda([this](FVectorVMExternalFunctionContext& Context) { this->GetFrostingDataVM(Context); });
    }

    if (BindingInfo.Name == GetSplatCountFuncName)
    {
        OutFunc = FVMExternalFunction::CreateLambda([this](FVectorVMExternalFunctionContext& Context) { this->GetSplatCountVM(Context); });
    }
}

#ifdef WITH_EDITORONLY_DATA
bool UNiagaraDataInterfaceFrostingData::AppendCompileHash(FNiagaraCompileHashVisitor* InVisitor) const
{
    if (!Super::AppendCompileHash(InVisitor))
    {
        return false;
    }
    InVisitor->UpdateShaderFile(GFrostingDITemplateShaderFile);
    InVisitor->UpdateShaderParameters<FGFrostingParameters>();
    return true;
}

bool UNiagaraDataInterfaceFrostingData::GetFunctionHLSL(const FNiagaraDataInterfaceGPUParamInfo& ParamInfo, const FNiagaraDataInterfaceGeneratedFunction& FunctionInfo, int FunctionInstanceIndex, FString& OutHLSL)
{
    return 
	FunctionInfo.DefinitionName == GetSplatDataFuncName
	|| FunctionInfo.DefinitionName == GetSplatCountFuncName
	|| FunctionInfo.DefinitionName == GetFrostingBarycentricFuncName;
}

void UNiagaraDataInterfaceFrostingData::GetParameterDefinitionHLSL(const FNiagaraDataInterfaceGPUParamInfo& ParamInfo, FString& OutHLSL)
{
    const TMap<FString, FStringFormatArg> TemplateArgs =
    {
        {TEXT("ParameterName"),	ParamInfo.DataInterfaceHLSLSymbol},
    };
    AppendTemplateHLSL(OutHLSL, GFrostingDITemplateShaderFile, TemplateArgs);
}
#endif

bool UNiagaraDataInterfaceFrostingData::Equals(const UNiagaraDataInterface* Other) const
{
    if (!Super::Equals(Other))
    {
        return false;
    }
    const UNiagaraDataInterfaceFrostingData* OtherTyped = CastChecked<const UNiagaraDataInterfaceFrostingData>(Other);

    return  OtherTyped->FrostingDataSource == FrostingDataSource;
}

void UNiagaraDataInterfaceFrostingData::BuildShaderParameters(FNiagaraShaderParametersBuilder& ShaderParametersBuilder) const
{
    ShaderParametersBuilder.AddNestedStruct<FGFrostingParameters>();
}

void UNiagaraDataInterfaceFrostingData::SetShaderParameters(const FNiagaraDataInterfaceSetShaderParametersContext& Context) const
{
    // Initializing the shader parameters to be the same reference of 
  //our buffers in the proxy
    FGFrostingParameters* ShaderParameters =
        Context.GetParameterNestedStruct<FGFrostingParameters>();
    if (ShaderParameters)
    {
        FRDGBuilder& GraphBuilder = Context.GetGraphBuilder();
        FNDIGaussianFrostingProxy& DataInterfaceProxy = Context.GetProxy<FNDIGaussianFrostingProxy>();
        auto& InstanceData = DataInterfaceProxy.PerInstanceData_RenderThread.FindChecked(Context.GetSystemInstanceID());

        // Constants
        ShaderParameters->FrostingSplatCount = InstanceData.NumElements;
        /// Assign initialized buffers to shader parameters

        InstanceData.AssetBuffer.Register(GraphBuilder);

        ShaderParameters->Barycentrics = InstanceData.AssetBuffer.PrismBarycentric.RDGTransientBufferSRV;
        ShaderParameters->FaceIndex = InstanceData.AssetBuffer.FaceIndices.RDGTransientBufferSRV;

        ShaderParameters->Scales = InstanceData.AssetBuffer.Scales.RDGTransientBufferSRV;
        ShaderParameters->Orientations = InstanceData.AssetBuffer.Orientations.RDGTransientBufferSRV;
        //ShaderParameters->Normals = InstanceData.AssetBuffer.Normals.SRV;
        ShaderParameters->SHAndOpacities = InstanceData.AssetBuffer.SHAndOpacities.RDGTransientBufferSRV;
    }
}

bool UNiagaraDataInterfaceFrostingData::CopyToInternal(UNiagaraDataInterface* Destination) const
{
    if (!Super::CopyToInternal(Destination))
    {
        return false;
    }

    UNiagaraDataInterfaceFrostingData* OtherTyped = CastChecked<UNiagaraDataInterfaceFrostingData>(Destination);

    OtherTyped->FrostingDataSource = FrostingDataSource;
    return true;
}

bool UNiagaraDataInterfaceFrostingData::InitPerInstanceData(void* PerInstanceData,
                                                            FNiagaraSystemInstance* SystemInstance)
{
    FNDIGaussianFrostingData_GameThread* InstanceData = new (PerInstanceData) FNDIGaussianFrostingData_GameThread();

    check(InstanceData);
    InstanceData->Init(this, SystemInstance);

    if (IsUsedWithGPUScript())
    {
        InstanceData->bUsedWithGpu = true;

        FNDIGaussianFrostingProxy* Proxy_GT = GetProxyAs<FNDIGaussianFrostingProxy>();
        Proxy_GT->PerInstanceData_GameThread.Add(SystemInstance->GetId(), InstanceData);
    }


    return true;
}

void UNiagaraDataInterfaceFrostingData::DestroyPerInstanceData(void* PerInstanceData,
	FNiagaraSystemInstance* SystemInstance)
{
    FNDIGaussianFrostingData_GameThread* InstanceData = static_cast<FNDIGaussianFrostingData_GameThread*>(PerInstanceData);

    if (InstanceData->bUsedWithGpu)
    {
        FNDIGaussianFrostingProxy* Proxy_GT = GetProxyAs<FNDIGaussianFrostingProxy>();
        Proxy_GT->PerInstanceData_GameThread.Remove(SystemInstance->GetId());

        ENQUEUE_RENDER_COMMAND(FNDIMemoryBuffer_RemoveProxy)
            (
                [RT_Proxy = Proxy_GT, InstanceID = SystemInstance->GetId()](FRHICommandListImmediate& CmdList)
                {
                    RT_Proxy->PerInstanceData_RenderThread.Remove(InstanceID);
                }
                );
    }

    InstanceData->~FNDIGaussianFrostingData_GameThread();
}


bool UNiagaraDataInterfaceFrostingData::PerInstanceTick(void* PerInstanceData, FNiagaraSystemInstance* SystemInstance,
	float DeltaSeconds)
{
    FNDIGaussianFrostingData_GameThread* InstanceData = reinterpret_cast<FNDIGaussianFrostingData_GameThread*>(PerInstanceData);

    if (InstanceData->bUsedWithGpu && InstanceData->bNeedsGpuSync)
    {
        ENQUEUE_RENDER_COMMAND(FNDIMemoryBuffer_UpdateData)
            (
                [RT_Proxy = GetProxyAs<FNDIGaussianFrostingProxy>(), RT_SystemInstanceID = SystemInstance->GetId(), RT_GameToRenderData = FGameToRenderData_Frosting(*InstanceData)](FRHICommandListImmediate& RHICmdList) mutable
                {
                    FNDIGaussianFrostingData_RenderThread& InstanceData_RT = RT_Proxy->PerInstanceData_RenderThread.FindOrAdd(RT_SystemInstanceID);
                    InstanceData_RT.UpdateData(RT_GameToRenderData);
                }
                );

        InstanceData->bNeedsGpuSync = false;
    }

    return false;
}

void UNiagaraDataInterfaceFrostingData::GetFrostingDataVM(FVectorVMExternalFunctionContext& Context)
{
    // Input is the NDI and Index of the particle
    VectorVM::FUserPtrHandler<FNDIGaussianFrostingData_GameThread>
        InstData(Context);

    FNDIInputParam<int32> IndexParam(Context);

    // Output Position
    //FNDIOutputParam<FPrismBarycentric> Barycentrics(Context);
    //VectorVM::FExternalFuncRegisterHandler<float> Barycentric0(Context);
    //VectorVM::FExternalFuncRegisterHandler<float> Barycentric1(Context);
    //VectorVM::FExternalFuncRegisterHandler<float> Barycentric2(Context);
    //VectorVM::FExternalFuncRegisterHandler<float> Barycentric3(Context);
    //VectorVM::FExternalFuncRegisterHandler<float> Barycentric4(Context);
    //VectorVM::FExternalFuncRegisterHandler<float> Barycentric5(Context);


    FNDIOutputParam<int32> FaceIndex(Context);
    //FNDIOutputParam<FVector3f> Normals(Context);
    FNDIOutputParam<FVector3f> Scales(Context);
    FNDIOutputParam<FQuat4f> Orientations(Context);
    FNDIOutputParam<FVector4f> SHAndOpacities(Context);

    auto& data = InstData->AssetArrays;

    const auto InstancesCount = Context.GetNumInstances();

    for (int32 i = 0; i < InstancesCount; ++i)
    {
        const int32 Index = IndexParam.GetAndAdvance();

        if (data.FaceIndices.IsValidIndex(Index))
        {
            FaceIndex.SetAndAdvance(data.FaceIndices[Index]);
            //Barycentrics.SetAndAdvance(data.BarryCoords[Index]);
            //*Barycentric0.GetDestAndAdvance() = data.BarryCoords[Index].a0;
            //*Barycentric1.GetDestAndAdvance() = data.BarryCoords[Index].a1;
            //*Barycentric2.GetDestAndAdvance() = data.BarryCoords[Index].a2;
            //*Barycentric3.GetDestAndAdvance() = data.BarryCoords[Index].a3;
            //*Barycentric4.GetDestAndAdvance() = data.BarryCoords[Index].a4;
            //*Barycentric5.GetDestAndAdvance() = data.BarryCoords[Index].a5;

            //Normals.SetAndAdvance(data.Normals[Index]);
            Scales.SetAndAdvance(data.Scales[Index]);
            Orientations.SetAndAdvance(data.Orientations[Index]);
        	SHAndOpacities.SetAndAdvance(data.SH0AndOpacities[Index]);
        }
        else
        {
            FaceIndex.SetAndAdvance(0);

            //*Barycentric0.GetDestAndAdvance() = data.BarryCoords[Index].a0;
            //*Barycentric1.GetDestAndAdvance() = data.BarryCoords[Index].a1;
            //*Barycentric2.GetDestAndAdvance() = data.BarryCoords[Index].a2;
            //*Barycentric3.GetDestAndAdvance() = data.BarryCoords[Index].a3;
            //*Barycentric4.GetDestAndAdvance() = data.BarryCoords[Index].a4;
            //*Barycentric5.GetDestAndAdvance() = data.BarryCoords[Index].a5;

			//Barycentrics.SetAndAdvance(FPrismBarycentric());
            //Normals.SetAndAdvance(FVector3f::ZeroVector);
            Scales.SetAndAdvance(FVector3f::ZeroVector);
            Orientations.SetAndAdvance(FQuat4f::Identity);
            SHAndOpacities.SetAndAdvance(FVector4f::Zero());
        }
    }
}

void UNiagaraDataInterfaceFrostingData::GetSplatCountVM(FVectorVMExternalFunctionContext& Context)
{
    // Input is the NDI and Index of the particle
    VectorVM::FUserPtrHandler<FNDIGaussianFrostingData_GameThread>
        InstData(Context);

    // Output Position
    FNDIOutputParam<int> OutCount(Context);

    const auto InstancesCount = Context.GetNumInstances();

    for (int32 i = 0; i < InstancesCount; ++i)
    {
        OutCount.SetAndAdvance(InstData.Get()->AssetArrays.SplatCount);
    }
}

void FNDIGaussianFrostingProxy::ConsumePerInstanceDataFromGameThread(void* PerInstanceData,
	const FNiagaraSystemInstanceID& Instance)
{
    checkNoEntry();
}

void FNDIGaussianFrostingProxy::PreStage(const FNDIGpuComputePreStageContext& Context)
{
    const FNiagaraSystemInstanceID SystemInstanceID = Context.GetSystemInstanceID();
    FNDIGaussianFrostingData_RenderThread& InstanceData = PerInstanceData_RenderThread.FindChecked(SystemInstanceID);
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

void FNDIGaussianFrostingProxy::PostSimulate(const FNDIGpuComputePostSimulateContext& Context)
{
	if (!Context.IsFinalPostSimulate())
	{
		return;
	}

	const FNiagaraSystemInstanceID SystemInstanceID = Context.GetSystemInstanceID();
	FNDIGaussianFrostingData_RenderThread& InstanceData = PerInstanceData_RenderThread.FindChecked(SystemInstanceID);

	// Clear out transient pointers we cache for graph usage
    InstanceData.AssetBuffer.ClearTransient();

 
}

#undef LOCTEXT_NAMESPACE