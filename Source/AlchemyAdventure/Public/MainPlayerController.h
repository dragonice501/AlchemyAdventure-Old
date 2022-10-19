// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "MainPlayerController.generated.h"

/**
 * 
 */
UCLASS()
class ALCHEMYADVENTURE_API AMainPlayerController : public APlayerController
{
	GENERATED_BODY()
	
public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Widgets") TSubclassOf<class UUserWidget> HUDOverlayAsset;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Widgets") UUserWidget* HUDOverlay;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Widgets") TSubclassOf<UUserWidget> WLockOnWidget;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Widgets") UUserWidget* LockOnWidget;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Widgets") TSubclassOf<UUserWidget> WPickupPromptWidget;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Widgets") UUserWidget* PickupPromptWidget;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Widgets") FString PickupPromptText = "";

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Widgets") TSubclassOf<UUserWidget> WPlayerInventoryWidget;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Widgets") UUserWidget* PlayerInventoryWidget;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Widgets") TSubclassOf<UUserWidget> WPlayerStatsWidget;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Widgets") UUserWidget* PlayerStatsWidget;

	FVector LockOnPosition;

	bool bPauseMenuVisible = false;
	bool bInventoryVisible = false;

public:

	// Called every frame
	virtual void Tick(float DeltaTime) override;

	void ToggleInventory(bool Visible);

	void DisplayLockOn(FVector Position);
	void RemoveLockOn();

	void DisplayPickupPrompt(FString PromptText);
	void RemovePickupPrompt();

	void DisplayStats();
	void RemoveStats();

	UFUNCTION(BlueprintCallable)
	void SaveGame();
	UFUNCTION(BlueprintCallable)
	void LoadGame();

protected:

	virtual void BeginPlay() override;
};
