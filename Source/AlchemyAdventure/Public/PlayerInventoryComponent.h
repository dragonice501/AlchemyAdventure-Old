// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Resource.h"
#include "Usable.h"
#include "Weapon.h"
#include "PlayerInventoryComponent.generated.h"

USTRUCT(BlueprintType)
struct FWeaponPropertyTable : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite) TSubclassOf<AWeapon> weaponBlueprint;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) UTexture2D* weaponImage;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) EWeaponType weaponType;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float damage;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float staminaCost;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float staminaRechargeDelay;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float poiseCost;
};

USTRUCT(BlueprintType)
struct FR
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite) FString resourceName;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FString resourceDescription;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FString> combinableResources;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FString> combineResults;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) UTexture2D* resourceImage;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) EResourceElement resourceElement;

	FR() : resourceName(""), resourceImage(nullptr), resourceElement(EResourceElement::ERE_None) {}

	bool operator== (const FR& other)
	{
		return resourceName == other.resourceName;
	}
	friend bool operator== (const FR& a, const FR& b)
	{
		return a.resourceName == b.resourceName;
	}
	friend uint32 GetTypeHash(const FR& other)
	{
		return GetTypeHash(other.resourceName);
	}

	void BuildResource(const FResourcePropertyTable* row)
	{
		if (row)
		{
			resourceName = row->resourceName;
			resourceDescription = row->resourceDescription;
			combinableResources = row->combinableResources;
			combineResults = row->combineResults;
			resourceImage = row->resourceImage;
			resourceElement = row->resourceElement;
		}
	}
};

USTRUCT(BlueprintType)
struct FU
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite) FString usableName;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FString usableDescription;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) UTexture2D* usableImage;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) EUsableElement usableElement;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) EStatusEffect statusEffect;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 statusEffectTime;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float attackModifier;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float healAmount;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float defenseModifier;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float mobilityModifier;

	FU() : usableName(""), usableImage(nullptr), usableElement(EUsableElement::ERE_None) {}

	bool operator== (const FU& other)
	{
		return usableName == other.usableName;
	}
	friend bool operator== (const FU& a, const FU& b)
	{
		return a.usableName == b.usableName;
	}
	friend uint32 GetTypeHash(const FU& other)
	{
		return GetTypeHash(other.usableName);
	}

	void BuildUsable(const FUsablePropertyTable* row)
	{
		if (row)
		{
			usableName = row->usableName;
			usableDescription = row->usableDescription;
			usableImage = row->usableImage;
			statusEffect = row->statusEffect;
			statusEffectTime = row->statusEffectTime;

			switch (statusEffect)
			{
			case EStatusEffect::ESE_None:
				break;
			case EStatusEffect::ESE_Attack:
				attackModifier = row->attackModifier;
				break;
			case EStatusEffect::ESE_Heal:
				healAmount = row->healAmount;
				break;
			case EStatusEffect::ESE_Defense:
				defenseModifier = row->defenseModifier;
				break;
			case EStatusEffect::ESE_Mobility:
				mobilityModifier = row->mobilityModifier;
				break;
			default:
				break;
			}
		}
	}
};

USTRUCT(BlueprintType)
struct FW
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite) TSubclassOf<AWeapon> weaponBlueprint;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FString weaponName;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) UTexture2D* weaponImage;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) EWeaponType weaponType;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float damage;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float staminaCost;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float staminaRechargeDelay;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float poiseCost;

	FW() : weaponName(""), weaponType(EWeaponType::EWT_Sword) {}

	/*bool operator> (const FW& other)
	{
		return weaponType > other.weaponType;
	}*/
	bool operator== (const FW& other)
	{
		return weaponName == other.weaponName;
	}
	friend bool operator== (const FW& a, const FW& b)
	{
		return a.weaponName == b.weaponName;
	}
	friend uint32 GetTypeHash(const FW& other)
	{
		return GetTypeHash(other.weaponName);
	}

	void BuildWeapon(const FWeaponPropertyTable* row, const FName& name)
	{
		if (row)
		{
			weaponBlueprint = row->weaponBlueprint;
			weaponName = name.ToString();
			weaponImage = row->weaponImage;
			weaponType = row->weaponType;
			damage = row->damage;
			staminaCost = row->staminaCost;
			staminaRechargeDelay = row->staminaRechargeDelay;
			poiseCost = row->poiseCost;

			if (weaponImage) UE_LOG(LogTemp, Warning, TEXT("Image"));
		}
	}
};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class ALCHEMYADVENTURE_API UPlayerInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// Inventory
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<AResource*> resourceInventory;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<AUsable*> usablesInventory;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<TSubclassOf<AWeapon>> weaponInventory;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<TSubclassOf<AUsable>> PotionMap;

	UPROPERTY(EditAnywhere, BlueprintReadWrite) TMap<FR, uint8> resourceMap;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TMap<FR, uint8> firstIngredientMap;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TMap<FR, uint8> secondIngredientMap;


	// Crafting
	UPROPERTY(VisibleAnywhere) TArray<AResource*> setIngredientsOneInv;
	UPROPERTY(VisibleAnywhere) TArray<AResource*> setIngredientsTwoInv;

	UPROPERTY(EditAnywhere, BlueprintReadWrite) FDataTableRowHandle usableDataTable;

	// Gear
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FW> weaponMap;

	UPROPERTY(EditAnywhere, BlueprintReadWrite) TMap<FU, uint8> usableMap;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TMap<FU, uint8> gearSlotOneMap;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TMap<FU, uint8> gearSlotTwoMap;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TMap<FU, uint8> gearSlotThreeMap;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TMap<FU, uint8> gearSlotFourMap;

	UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<int32> currentGearIndexes{ -1, -1, -1, -1 };
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<AUsable*> gearSlotOneInventory;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<AUsable*> gearSlotTwoInventory;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<AUsable*> gearSlotThreeInventory;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<AUsable*> gearSlotFourInventory;

public:	
	// Sets default values for this component's properties
	UPlayerInventoryComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// Inventory
	void AddToResourceInventory(AResource* resource) { resourceInventory.Add(resource); }
	void AddToResourceInventory(const FDataTableRowHandle& itemDataTable);
	void AddToResourceInventory(const FR& resource, uint8 count);
	void AddToUsuablesInventory(AUsable* usable) { usablesInventory.Add(usable); }
	void AddToUsablesInventory(const FU& usable, const uint8 count);
	void AddToWeaponInventory(TSubclassOf<AWeapon> weapon);
	void AddToWeaponInventory(const FDataTableRowHandle& weaponDataTable);

	void RemoveResourceFromInventory(const FR& resource, uint8 count);

	//void SortResourceInventory() { resourceMap.KeySort(); }

	UFUNCTION(BlueprintCallable) bool RemoveAndSetIngredient(int32 resourceStackIndex, int32 resourceSelectIndex, bool firstIngredient, UTexture2D*& resourceImage);
	UFUNCTION(BlueprintCallable) void ResetCraftingIngredients(bool resetFirst, bool resetSecond);

	UFUNCTION(BlueprintCallable) void GetResource(int32 resourceStackIndex, int32 inUseIngredientIndex, int32& resourceInventoryIndex, UTexture2D*& resourceImage, bool& hasResource);
	UFUNCTION(BlueprintCallable) void GetResourceImage(int32 resourceStackIndex, UTexture2D*& resourceImage);
	UFUNCTION(BlueprintCallable) void GetResourceCount(int32 resourceStackIndex, int32& resourceCount);

	UFUNCTION(BlueprintCallable) UTexture2D* CheckCanCraft();
	UFUNCTION(BlueprintCallable) bool AddUsable();
	UFUNCTION(BlueprintCallable) bool CheckCanCraftMore();
	UFUNCTION(BlueprintCallable) int32 GetIngredientsCount(bool first, bool second);
	UFUNCTION(BlueprintCallable) UTexture2D* GetIngredientImage(bool first, bool second);

	UFUNCTION(BlueprintCallable) UTexture2D* GetInventoryWeaponImage(int32 index);
	UFUNCTION(BlueprintCallable) UTexture2D* GetGearImage(int32 index, int32& itemCount);

	// Gear
	UFUNCTION(BlueprintCallable) void SetGearUseIndex(int32 gearBoxIndex, int32 desiredInventoryIndex);
	UFUNCTION(BlueprintCallable) void SetGearSlot(int32 stackIndex, int32 slotIndex);
	UFUNCTION(BlueprintCallable) void RemoveGearAtSlot(int32 gearSlotIndex);
	UFUNCTION(BlueprintCallable) void SwapGearSlot(int32 firstSlot, int32 secondSlot);
	UFUNCTION(BlueprintCallable) void GetGearSlotImageAndCount(int32 slotIndex, UTexture2D*& outImage, int32& count, bool& hasGear);
	UFUNCTION(BlueprintCallable) void GetGearInventoryStackImageAndCount(int32 stackIndex, UTexture2D*& outImage, int32& count, bool& hasGear);
	void AddToGearSlot(AUsable* usableToAdd, int32 slotIndex);

	/*void QuickSortUsables(TArray<AUsable*> Inventory, int32 Low, int32 High);
	int32 Partition(TArray<AUsable*> Inventory, int32 Low, int32 High);
	void Swap(int32 i, int32 j);
	void DutchQuickSort(TArray<AUsable*> Inventory, int Left, int Right);
	void DutchPartition(TArray<AUsable*> Arr, int Left, int Right, int i, int j);*/
};
