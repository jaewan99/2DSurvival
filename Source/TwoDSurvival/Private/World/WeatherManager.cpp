// Fill out your copyright notice in the Description page of Project Settings.

#include "World/WeatherManager.h"
#include "World/TimeManager.h"
#include "Character/BaseCharacter.h"
#include "Components/NeedsComponent.h"
#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"

AWeatherManager::AWeatherManager()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AWeatherManager::BeginPlay()
{
	Super::BeginPlay();

	// Create a looping audio component for ambient weather sounds.
	AmbientAudioComp = NewObject<UAudioComponent>(this, TEXT("WeatherAmbient"));
	AmbientAudioComp->RegisterComponent();
	AmbientAudioComp->bAutoActivate = false;
	AmbientAudioComp->bIsUISound = false;

	// Cache the TimeManager so we can push fog/tint overrides to it.
	TArray<AActor*> Managers;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ATimeManager::StaticClass(), Managers);
	if (Managers.Num() > 0)
	{
		CachedTimeManager = Cast<ATimeManager>(Managers[0]);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[WeatherManager] No ATimeManager found in level — weather visuals disabled."));
	}

	SelectNextWeatherState();
	ApplyWeatherToTimeManager();
	UpdateNeedsModifiers();
}

void AWeatherManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	StateElapsedTime += DeltaTime;
	if (StateElapsedTime >= StateDuration)
	{
		SelectNextWeatherState();
		ApplyWeatherToTimeManager();
		UpdateNeedsModifiers();
	}
}

// ---------------------------------------------------------------------------
// State selection
// ---------------------------------------------------------------------------

EWeatherState AWeatherManager::PickWeightedState(ESeason Season)
{
	struct FEntry { EWeatherState State; float Weight; };

	TArray<FEntry> Table;
	switch (Season)
	{
	case ESeason::Spring:
		Table = {
			{EWeatherState::Clear,  40.f},
			{EWeatherState::Cloudy, 40.f},
			{EWeatherState::Rain,   20.f},
		};
		break;
	case ESeason::Summer:
		Table = {
			{EWeatherState::Clear,     40.f},
			{EWeatherState::Cloudy,    25.f},
			{EWeatherState::Rain,      25.f},
			{EWeatherState::HeavyRain, 10.f},
		};
		break;
	case ESeason::Fall:
		Table = {
			{EWeatherState::Clear,  30.f},
			{EWeatherState::Cloudy, 40.f},
			{EWeatherState::Rain,   30.f},
		};
		break;
	case ESeason::Winter:
		Table = {
			{EWeatherState::Clear,  40.f},
			{EWeatherState::Cloudy, 40.f},
			{EWeatherState::Snow,   20.f},
		};
		break;
	default:
		return EWeatherState::Clear;
	}

	float Total = 0.f;
	for (const FEntry& E : Table) Total += E.Weight;

	float Roll = FMath::FRandRange(0.f, Total);
	float Accum = 0.f;
	for (const FEntry& E : Table)
	{
		Accum += E.Weight;
		if (Roll <= Accum) return E.State;
	}
	return Table.Last().State;
}

void AWeatherManager::SelectNextWeatherState()
{
	const ESeason Season = CachedTimeManager
		? CachedTimeManager->GetCurrentSeason()
		: ESeason::Spring;

	CurrentWeather     = PickWeightedState(Season);
	StateDuration      = FMath::FRandRange(MinStateDurationSeconds, MaxStateDurationSeconds);
	StateElapsedTime   = 0.f;

	bIsRaining = (CurrentWeather == EWeatherState::Rain || CurrentWeather == EWeatherState::HeavyRain);
	bIsSnowing = (CurrentWeather == EWeatherState::Snow);

	// Play the matching ambient sound (or silence if Clear/Cloudy).
	USoundBase* Sound = nullptr;
	switch (CurrentWeather)
	{
	case EWeatherState::Rain:      Sound = SFX_Rain;      break;
	case EWeatherState::HeavyRain: Sound = SFX_HeavyRain; break;
	case EWeatherState::Snow:      Sound = SFX_Snow;      break;
	default: break;
	}
	PlayWeatherSound(Sound);

	OnWeatherChanged.Broadcast(CurrentWeather);

	static const TCHAR* Names[] = { TEXT("Clear"), TEXT("Cloudy"), TEXT("Rain"), TEXT("HeavyRain"), TEXT("Snow") };
	UE_LOG(LogTemp, Log, TEXT("[WeatherManager] State → %s (season: %d, duration: %.0fs)"),
		Names[(int32)CurrentWeather], (int32)Season, StateDuration);
}

// ---------------------------------------------------------------------------
// Visuals — pushed to TimeManager's shared PP + fog components
// ---------------------------------------------------------------------------

void AWeatherManager::ApplyWeatherToTimeManager()
{
	if (!CachedTimeManager) return;

	float FogAdd = 0.f;
	FLinearColor Tint = FLinearColor::White;

	switch (CurrentWeather)
	{
	case EWeatherState::Cloudy:    FogAdd = CloudyFogAdd;    Tint = CloudyTint;    break;
	case EWeatherState::Rain:      FogAdd = RainFogAdd;      Tint = RainTint;      break;
	case EWeatherState::HeavyRain: FogAdd = HeavyRainFogAdd; Tint = HeavyRainTint; break;
	case EWeatherState::Snow:      FogAdd = SnowFogAdd;      Tint = SnowTint;      break;
	default: break;
	}

	CachedTimeManager->SetWeatherFogOffset(FogAdd);
	CachedTimeManager->SetWeatherTint(Tint);
}

// ---------------------------------------------------------------------------
// Needs modifiers — pushed to all ABaseCharacter::NeedsComponent instances
// ---------------------------------------------------------------------------

void AWeatherManager::UpdateNeedsModifiers()
{
	const float ThirstBoost = GetThirstBoostRate();
	const float MoodDrain   = GetMoodDrainRate();

	TArray<AActor*> Players;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ABaseCharacter::StaticClass(), Players);
	for (AActor* A : Players)
	{
		ABaseCharacter* Player = Cast<ABaseCharacter>(A);
		if (Player && Player->NeedsComponent)
		{
			Player->NeedsComponent->SetWeatherModifiers(ThirstBoost, MoodDrain);
		}
	}
}

float AWeatherManager::GetThirstBoostRate() const
{
	switch (CurrentWeather)
	{
	case EWeatherState::Rain:      return RainThirstBoost;
	case EWeatherState::HeavyRain: return HeavyRainThirstBoost;
	default: return 0.f;
	}
}

float AWeatherManager::GetMoodDrainRate() const
{
	switch (CurrentWeather)
	{
	case EWeatherState::Rain:      return RainMoodDrain;
	case EWeatherState::HeavyRain: return HeavyRainMoodDrain;
	case EWeatherState::Snow:      return SnowMoodDrain;
	default: return 0.f;
	}
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

void AWeatherManager::PlayWeatherSound(USoundBase* Sound)
{
	if (!AmbientAudioComp) return;
	AmbientAudioComp->Stop();
	if (Sound)
	{
		AmbientAudioComp->SetSound(Sound);
		AmbientAudioComp->Play();
	}
}

void AWeatherManager::RestoreWeather(EWeatherState State, float ElapsedTime)
{
	CurrentWeather   = State;
	StateElapsedTime = ElapsedTime;
	StateDuration    = FMath::FRandRange(MinStateDurationSeconds, MaxStateDurationSeconds);

	// Ensure remaining time is non-negative.
	if (StateElapsedTime >= StateDuration) StateElapsedTime = 0.f;

	bIsRaining = (CurrentWeather == EWeatherState::Rain || CurrentWeather == EWeatherState::HeavyRain);
	bIsSnowing = (CurrentWeather == EWeatherState::Snow);

	ApplyWeatherToTimeManager();
	UpdateNeedsModifiers();

	// Restore ambient sound.
	USoundBase* Sound = nullptr;
	switch (CurrentWeather)
	{
	case EWeatherState::Rain:      Sound = SFX_Rain;      break;
	case EWeatherState::HeavyRain: Sound = SFX_HeavyRain; break;
	case EWeatherState::Snow:      Sound = SFX_Snow;      break;
	default: break;
	}
	PlayWeatherSound(Sound);
}
