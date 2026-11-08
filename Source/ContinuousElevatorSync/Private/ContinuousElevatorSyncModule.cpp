#include "ContinuousElevatorSyncModule.h"

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "FGBuildableSubsystem.h"
#include "Subsystems/WorldSubsystem.h"
#include "UObject/UnrealType.h"

DEFINE_LOG_CATEGORY(LogContinuousElevatorSync);

namespace
{
	// The Continuous Elevator mod's world subsystem and its countdown variable, found in its cooked Blueprint.
	const TCHAR* const SubsystemClassPath = TEXT("/KeepFeedingToMaintainPhase/KFMP_Sub.KFMP_Sub_C");
	const FName CountdownPropertyName(TEXT("Int"));

	// The Blueprint counts down one step per world-time second from ConsumeSteps and consumes when it reaches 0.
	constexpr int32 ConsumeSteps = 60;
	constexpr float ConsumeSeconds = 60.0f;
	// Value held in the countdown while factory time has not yet run out.
	constexpr int32 HoldValue = 1000;
	// Value that makes the Blueprint's next step reach 0 and consume.
	constexpr int32 TriggerValue = 1;
	// A factory-clock jump larger than this is a load or a hitch, not elapsed factory time.
	constexpr float MaxFactoryStep = 10.0f;
	constexpr float FindRetrySeconds = 2.0f;
	constexpr float LogIntervalSeconds = 60.0f;

	struct FSyncState
	{
		TWeakObjectPtr<UObject> Subsystem;
		bool bSeeded = false;
		bool bTriggered = false;
		float LastFactoryTime = 0.0f;
		float FactoryAccumulator = 0.0f;
		float RealSinceFind = 0.0f;
		float LogTimer = 0.0f;
		float LogFactory = 0.0f;
		float LogWorld = 0.0f;
		int32 Consumptions = 0;
		bool bLoggedProblem = false;
	};

	FSyncState State;

	UWorld* FindServerGameWorld()
	{
		if (!GEngine)
		{
			return nullptr;
		}
		for (const FWorldContext& Context : GEngine->GetWorldContexts())
		{
			UWorld* World = Context.World();
			if (Context.WorldType == EWorldType::Game && World && World->GetNetMode() != NM_Client)
			{
				return World;
			}
		}
		return nullptr;
	}
}

void FContinuousElevatorSyncModule::StartupModule()
{
	if (WITH_EDITOR)
	{
		return;
	}
	TickHandle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateRaw(this, &FContinuousElevatorSyncModule::Tick));
	UE_LOG(LogContinuousElevatorSync, Log, TEXT("Started: Continuous Elevator consumption will follow factory time on this server."));
}

void FContinuousElevatorSyncModule::ShutdownModule()
{
	FTSTicker::GetCoreTicker().RemoveTicker(TickHandle);
}

bool FContinuousElevatorSyncModule::Tick(float DeltaTime)
{
	UWorld* World = FindServerGameWorld();
	if (!World)
	{
		State = FSyncState();
		return true;
	}

	UClass* SubsystemClass = FindObject<UClass>(nullptr, SubsystemClassPath);
	if (!SubsystemClass)
	{
		if (!State.bLoggedProblem)
		{
			State.bLoggedProblem = true;
			UE_LOG(LogContinuousElevatorSync, Log, TEXT("%s is not loaded; waiting (the Continuous Elevator mod may not be installed)."), SubsystemClassPath);
		}
		return true;
	}
	FIntProperty* Countdown = FindFProperty<FIntProperty>(SubsystemClass, CountdownPropertyName);
	if (!Countdown)
	{
		if (!State.bLoggedProblem)
		{
			State.bLoggedProblem = true;
			UE_LOG(LogContinuousElevatorSync, Error, TEXT("%s has no int property '%s'; the Continuous Elevator mod changed and this mod does nothing."), SubsystemClassPath, *CountdownPropertyName.ToString());
		}
		return true;
	}

	UObject* Subsystem = State.Subsystem.Get();
	if (!Subsystem)
	{
		State.RealSinceFind += DeltaTime;
		if (State.RealSinceFind < FindRetrySeconds)
		{
			return true;
		}
		UWorldSubsystem* Found = Cast<UWorldSubsystem>(World->GetSubsystemBase(SubsystemClass));
		if (!Found)
		{
			State.RealSinceFind = 0.0f;
			return true;
		}
		State = FSyncState();
		State.Subsystem = Found;
		Subsystem = Found;
		UE_LOG(LogContinuousElevatorSync, Log, TEXT("Found the Continuous Elevator subsystem %s."), *GetNameSafe(Subsystem));
	}

	AFGBuildableSubsystem* Buildables = AFGBuildableSubsystem::Get(World);
	if (!Buildables)
	{
		return true;
	}

	int32* CountdownPtr = Countdown->ContainerPtrToValuePtr<int32>(Subsystem);
	const float FactoryTime = Buildables->GetFactorySimulationTime();
	float FactoryStep = FactoryTime - State.LastFactoryTime;
	State.LastFactoryTime = FactoryTime;
	if (FactoryStep < 0.0f || FactoryStep > MaxFactoryStep)
	{
		FactoryStep = 0.0f;
	}

	if (!State.bSeeded)
	{
		// Resume the progress the Blueprint had already made (it is saved with the game).
		State.bSeeded = true;
		State.FactoryAccumulator = (*CountdownPtr >= 1 && *CountdownPtr <= ConsumeSteps) ? static_cast<float>(ConsumeSteps - *CountdownPtr) : 0.0f;
		UE_LOG(LogContinuousElevatorSync, Log, TEXT("Seeded: countdown %d, %.0f factory seconds already counted."), *CountdownPtr, State.FactoryAccumulator);
		return true;
	}

	State.FactoryAccumulator += FactoryStep;
	State.LogFactory += FactoryStep;
	State.LogWorld += DeltaTime;
	State.LogTimer += DeltaTime;

	if (State.bTriggered && *CountdownPtr == ConsumeSteps)
	{
		// The Blueprint consumed and reset its countdown.
		State.bTriggered = false;
		State.FactoryAccumulator = FMath::Max(0.0f, State.FactoryAccumulator - ConsumeSeconds);
		++State.Consumptions;
		UE_LOG(LogContinuousElevatorSync, Log, TEXT("Consumed (#%d); %.1f factory seconds carried over."), State.Consumptions, State.FactoryAccumulator);
	}

	if (!State.bTriggered)
	{
		if (State.FactoryAccumulator >= ConsumeSeconds)
		{
			State.bTriggered = true;
			*CountdownPtr = TriggerValue;
		}
		else if (*CountdownPtr != HoldValue)
		{
			*CountdownPtr = HoldValue;
		}
	}

	if (State.LogTimer >= LogIntervalSeconds)
	{
		UE_LOG(LogContinuousElevatorSync, Log, TEXT("Last %.0f s: factory time advanced %.1f s, world time %.1f s (factory/world %.2f). Next consume in %.1f factory s."),
			State.LogTimer, State.LogFactory, State.LogWorld, State.LogWorld > 0.0f ? State.LogFactory / State.LogWorld : 0.0f, FMath::Max(0.0f, ConsumeSeconds - State.FactoryAccumulator));
		State.LogTimer = State.LogFactory = State.LogWorld = 0.0f;
	}
	return true;
}

IMPLEMENT_MODULE(FContinuousElevatorSyncModule, ContinuousElevatorSync)
