﻿// Copyright Timothé Lapetite 2024
// Released under the MIT license https://opensource.org/license/MIT/

#pragma once

#ifndef PCGEX_MACROS
#define PCGEX_MACROS

#define PCGEX_NODE_INFOS(_SHORTNAME, _NAME, _TOOLTIP)\
virtual FName GetDefaultNodeName() const override { return FName(TEXT(#_SHORTNAME)); } \
virtual FName AdditionalTaskName() const override{ return bCacheResult ? FName(FString("* ")+GetDefaultNodeTitle().ToString()) : FName(GetDefaultNodeTitle().ToString()); }\
virtual FText GetDefaultNodeTitle() const override { return NSLOCTEXT("PCGEx" #_SHORTNAME, "NodeTitle", "PCGEx | " _NAME);} \
virtual FText GetNodeTooltipText() const override{ return NSLOCTEXT("PCGEx" #_SHORTNAME "Tooltip", "NodeTooltip", _TOOLTIP); }

#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION == 3
#define PCGEX_NODE_INFOS_CUSTOM_SUBTITLE(_SHORTNAME, _NAME, _TOOLTIP, _TASK_NAME)\
virtual FName GetDefaultNodeName() const override { return FName(TEXT(#_SHORTNAME)); } \
virtual FName AdditionalTaskName() const override{ return _TASK_NAME.IsNone() ? FName(GetDefaultNodeTitle().ToString()) : FName(FString(GetDefaultNodeTitle().ToString() + "\r" + _TASK_NAME.ToString())); }\
virtual FText GetDefaultNodeTitle() const override { return NSLOCTEXT("PCGEx" #_SHORTNAME, "NodeTitle", "PCGEx | " _NAME);} \
virtual FText GetNodeTooltipText() const override{ return NSLOCTEXT("PCGEx" #_SHORTNAME "Tooltip", "NodeTooltip", _TOOLTIP); }
#else
#define PCGEX_NODE_INFOS_CUSTOM_SUBTITLE(_SHORTNAME, _NAME, _TOOLTIP, _TASK_NAME)\
virtual FName GetDefaultNodeName() const override { return FName(TEXT(#_SHORTNAME)); } \
virtual FName AdditionalTaskName() const override{ return FName(GetDefaultNodeTitle().ToString()); }\
virtual FString GetAdditionalTitleInformation() const override{ return _TASK_NAME.IsNone() ? FString() : _TASK_NAME.ToString(); }\
virtual bool HasFlippedTitleLines() const { return !_TASK_NAME.IsNone(); }\
virtual FText GetDefaultNodeTitle() const override { return NSLOCTEXT("PCGEx" #_SHORTNAME, "NodeTitle", "PCGEx | " _NAME);} \
virtual FText GetNodeTooltipText() const override{ return NSLOCTEXT("PCGEx" #_SHORTNAME "Tooltip", "NodeTooltip", _TOOLTIP); }
#endif

#define PCGEX_INITIALIZE_CONTEXT(_NAME)\
FPCGContext* FPCGEx##_NAME##Element::Initialize( const FPCGDataCollection& InputData, TWeakObjectPtr<UPCGComponent> SourceComponent, const UPCGNode* Node)\
{	FPCGEx##_NAME##Context* Context = new FPCGEx##_NAME##Context();	return InitializeContext(Context, InputData, SourceComponent, Node); }
#define PCGEX_INITIALIZE_ELEMENT(_NAME)\
PCGEX_INITIALIZE_CONTEXT(_NAME)\
FPCGElementPtr UPCGEx##_NAME##Settings::CreateElement() const{	return MakeShared<FPCGEx##_NAME##Element>();}
#define PCGEX_CONTEXT(_NAME) FPCGEx##_NAME##Context* Context = static_cast<FPCGEx##_NAME##Context*>(InContext);
#define PCGEX_SETTINGS(_NAME) const UPCGEx##_NAME##Settings* Settings = Context->GetInputSettings<UPCGEx##_NAME##Settings>();	check(Settings);
#define PCGEX_SETTINGS_LOCAL(_NAME) const UPCGEx##_NAME##Settings* Settings = GetInputSettings<UPCGEx##_NAME##Settings>();	check(Settings);
#define PCGEX_CONTEXT_AND_SETTINGS(_NAME) PCGEX_CONTEXT(_NAME) PCGEX_SETTINGS(_NAME)
#define PCGEX_OPERATION_DEFAULT(_NAME, _TYPE)  // if(!_NAME){_NAME = NewObject<_TYPE>(this, TEXT(#_NAME), RF_Transactional); _NAME->UpdateUserFacingInfos();} //ObjectInitializer.CreateDefaultSubobject<_TYPE>(this, TEXT(#_NAME));
#define PCGEX_OPERATION_VALIDATE(_NAME) if(!Settings->_NAME){PCGE_LOG(Error, GraphAndLog, FTEXT("No operation selected for : "#_NAME)); return false;}
#define PCGEX_OPERATION_BIND(_NAME, _TYPE) PCGEX_OPERATION_VALIDATE(_NAME) Context->_NAME = Context->RegisterOperation<_TYPE>(Settings->_NAME);
#define PCGEX_VALIDATE_NAME(_NAME) if (!PCGEx::IsValidName(_NAME)){	PCGE_LOG(Error, GraphAndLog, FTEXT("Invalid user-defined attribute name for " #_NAME)); return false;	}
#define PCGEX_VALIDATE_NAME_C(_CTX, _NAME) if (!PCGEx::IsValidName(_NAME)){	PCGE_LOG_C(Error, GraphAndLog, _CTX, FTEXT("Invalid user-defined attribute name for " #_NAME)); return false;	}
#define PCGEX_SOFT_VALIDATE_NAME(_BOOL, _NAME, _CTX) if(_BOOL){if (!PCGEx::IsValidName(_NAME)){ PCGE_LOG_C(Warning, GraphAndLog, _CTX, FTEXT("Invalid user-defined attribute name for " #_NAME)); _BOOL = false; } }
#define PCGEX_FWD(_NAME) Context->_NAME = Settings->_NAME;
#define PCGEX_TERMINATE_ASYNC PCGEX_DELETE(AsyncManager)

#define PCGEX_TYPED_CONTEXT_AND_SETTINGS(_NAME) FPCGEx##_NAME##Context* TypedContext = GetContext<FPCGEx##_NAME##Context>(); const UPCGEx##_NAME##Settings* Settings = Context->GetInputSettings<UPCGEx##_NAME##Settings>();	check(Settings);

#define PCGEX_WAIT_ASYNC if (!Context->IsAsyncWorkComplete()) {return false;}

#if WITH_EDITOR
#define PCGEX_PIN_TOOLTIP(_TOOLTIP) Pin.Tooltip = FTEXT(_TOOLTIP);
#else
#define PCGEX_PIN_TOOLTIP(_TOOLTIP) {}
#endif

#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION > 3
#define PCGEX_PIN_STATUS(_STATUS) Pin.PinStatus = EPCGPinStatus::_STATUS;
#else
#define PCGEX_PIN_STATUS(_STATUS) Pin.bAdvancedPin = EPCGPinStatus::_STATUS == EPCGPinStatus::Advanced;
#endif

#define PCGEX_PIN_ANY(_LABEL, _TOOLTIP, _STATUS, _EXTRA) { FPCGPinProperties& Pin = PinProperties.Emplace_GetRef(_LABEL, EPCGDataType::Any); PCGEX_PIN_TOOLTIP(_TOOLTIP) PCGEX_PIN_STATUS(_STATUS) _EXTRA }
#define PCGEX_PIN_POINTS(_LABEL, _TOOLTIP, _STATUS, _EXTRA) { FPCGPinProperties& Pin = PinProperties.Emplace_GetRef(_LABEL, EPCGDataType::Point); PCGEX_PIN_TOOLTIP(_TOOLTIP) PCGEX_PIN_STATUS(_STATUS) _EXTRA }
#define PCGEX_PIN_POLYLINES(_LABEL, _TOOLTIP, _STATUS, _EXTRA) { FPCGPinProperties& Pin = PinProperties.Emplace_GetRef(_LABEL, EPCGDataType::PolyLine); PCGEX_PIN_TOOLTIP(_TOOLTIP) PCGEX_PIN_STATUS(_STATUS) _EXTRA }
#define PCGEX_PIN_PARAMS(_LABEL, _TOOLTIP, _STATUS, _EXTRA) { FPCGPinProperties& Pin = PinProperties.Emplace_GetRef(_LABEL, EPCGDataType::Param); PCGEX_PIN_TOOLTIP(_TOOLTIP) PCGEX_PIN_STATUS(_STATUS) _EXTRA }
#define PCGEX_PIN_POINT(_LABEL, _TOOLTIP, _STATUS, _EXTRA) { FPCGPinProperties& Pin = PinProperties.Emplace_GetRef(_LABEL, EPCGDataType::Point, false, true); PCGEX_PIN_TOOLTIP(_TOOLTIP) PCGEX_PIN_STATUS(_STATUS) _EXTRA }
#define PCGEX_PIN_PARAM(_LABEL, _TOOLTIP, _STATUS, _EXTRA) { FPCGPinProperties& Pin = PinProperties.Emplace_GetRef(_LABEL, EPCGDataType::Param, false, true); PCGEX_PIN_TOOLTIP(_TOOLTIP) PCGEX_PIN_STATUS(_STATUS) _EXTRA }

#endif
