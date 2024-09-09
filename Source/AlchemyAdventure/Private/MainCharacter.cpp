// Fill out your copyright notice in the Description page of Project Settings.

#include "MainCharacter.h"
#include "CharacterAttributesComponent.h"
#include "Enemy.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "MainPlayerController.h"
#include "Pickup.h"
#include "PlayerInventoryComponent.h"
#include "Resource.h"
#include "StatusEffect.h"
#include "StatusEffectsComponent.h"
#include "Usable.h"
#include "Weapon.h"
#include "Animation/AnimInstance.h"
#include "Camera/CameraComponent.h"
#include "Components/SphereComponent.h"
#include "Components/BoxComponent.h"
#include "Components/ArrowComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

// Sets default values
AMainCharacter::AMainCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArm->SetupAttachment(RootComponent);

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(SpringArm, USpringArmComponent::SocketName);
	Camera->bUsePawnControlRotation = false;

	EnemyDetectionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("EnemyDetectionSphere"));
	EnemyDetectionSphere->SetupAttachment(RootComponent);

	mAttributes = CreateDefaultSubobject<UCharacterAttributesComponent>(TEXT("CharacterAttributes"));
	mInventory = CreateDefaultSubobject<UPlayerInventoryComponent>(TEXT("PlayerInventory"));
	mStatusEffects = CreateDefaultSubobject<UStatusEffectsComponent>(TEXT("StatusEffects"));

	SwitchMovementStyle(EMovementStyle::EMS_Free);
	SwitchCameraMovement(ECameraMovement::ECM_Free);
}

// Called when the game starts or when spawned
void AMainCharacter::BeginPlay()
{
	Super::BeginPlay();

	EnemyDetectionSphere->OnComponentBeginOverlap.AddDynamic(this, &AMainCharacter::EnemyDetectionSphereBeginOverlap);
	EnemyDetectionSphere->OnComponentEndOverlap.AddDynamic(this, &AMainCharacter::EnemyDetectionSphereEndOverlap);

	MainPlayerController = Cast<AMainPlayerController>(GetController());

	if (StartingWeapon)
	{
		EquipRightHand(StartingWeapon);
	}

	if (APlayerController* playerController = Cast<APlayerController>(GetController()))
	{
		if (UEnhancedInputLocalPlayerSubsystem* subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(playerController->GetLocalPlayer()))
		{
			if (mMappingContext)
			{
				subsystem->AddMappingContext(mMappingContext, 0);
			}
		}
	}
}

// Called every frame
void AMainCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	GetCharacterMovement()->MaxWalkSpeed = MaxWalkSpeed;

	if (bLockedOn && TargetEnemy)
	{
		float SqrDistance = FVector::DistSquared(GetActorLocation(), TargetEnemy->GetActorLocation());
		if (SqrDistance > FMath::Square(MaxLockOnDistance))
		{
			UntargetEnemy();
			SwitchMovementStyle(EMovementStyle::EMS_Free);
			SwitchCameraMovement(ECameraMovement::ECM_Free);
			return;
		}

		LockOnEnemy();
		LookAtTarget();
	}
	else if (bInventoryOpen)
	{
		LookAtInventory();
	}

	if (!bInventoryOpen)
	{
		CheckOverlappingPickups();
	}
}

// Called to bind functionality to input
void AMainCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* input = CastChecked<UEnhancedInputComponent>(PlayerInputComponent))
	{
		input->BindAction(mMoveAction, ETriggerEvent::Triggered, this, &AMainCharacter::Move);
		input->BindAction(mLookAction, ETriggerEvent::Triggered, this, &AMainCharacter::Look);
		input->BindAction(mAttackAction, ETriggerEvent::Triggered, this, &AMainCharacter::Attack);
		input->BindAction(mStartBlockAction, ETriggerEvent::Triggered, this, &AMainCharacter::StartBlock);
		input->BindAction(mEndBlockAction, ETriggerEvent::Triggered, this, &AMainCharacter::EndBlock);
		input->BindAction(mDodgeAction, ETriggerEvent::Triggered, this, &AMainCharacter::Dodge);
		input->BindAction(mInteractAction, ETriggerEvent::Triggered, this, &AMainCharacter::Interact);
		input->BindAction(mMenuAction, ETriggerEvent::Triggered, this, &AMainCharacter::Menu);
		input->BindAction(mToggleLockOnAction, ETriggerEvent::Triggered, this, &AMainCharacter::ToggleLockOn);
		
		input->BindAction(mOneShortcutAction, ETriggerEvent::Triggered, this, &AMainCharacter::OnePressed);
		input->BindAction(mTwoShortcutAction, ETriggerEvent::Triggered, this, &AMainCharacter::TwoPressed);
		input->BindAction(mThreeShortcutAction, ETriggerEvent::Triggered, this, &AMainCharacter::ThreePressed);
		input->BindAction(mFourShortcutAction, ETriggerEvent::Triggered, this, &AMainCharacter::FourPressed);
	}
}

float AMainCharacter::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	if(mAttributes)
	{
		if (mAttributes->health - DamageAmount <= 0.0f)
		{
			mAttributes->health = 0.0f;
			Die(DamageCauser);
			return DamageAmount;
		}
		else
		{
			mAttributes->health -= DamageAmount;
		}
	}

	AWeapon* Weapon = Cast<AWeapon>(DamageCauser);
	if (Weapon)
	{
		if (mAttributes)
		{
			mAttributes->DepletePoise(Weapon->poiseCost);
		}
	}

	return DamageAmount;
}

void AMainCharacter::Move(const FInputActionValue& value)
{
	FVector2D moveVector = value.Get<FVector2D>();
	mInputVector.X = moveVector.X;
	mInputVector.Y = moveVector.Y;

	if (mInputVector.SquaredLength() > 0.0f)
	{
		const FRotator rotation = GetControlRotation();
		const FRotator yaw(0.0f, rotation.Yaw, 0.0f);

		const FVector forward = FRotationMatrix(yaw).GetUnitAxis(EAxis::X);
		const FVector right = FRotationMatrix(yaw).GetUnitAxis(EAxis::Y);

		AddMovementInput(forward, mInputVector.X);
		AddMovementInput(right, mInputVector.Y);
	}
}

void AMainCharacter::Look(const FInputActionValue& value)
{
	FVector2D lookVector = value.Get<FVector2D>();

	if (lookVector.SquaredLength() > 0.0f)
	{
		AddControllerPitchInput(lookVector.Y);
		AddControllerYawInput(lookVector.X);
	}
}

void AMainCharacter::Menu(const FInputActionValue& value)
{
	if (MainPlayerController)
	{
		bBlocking = false;

		bInventoryOpen = !bInventoryOpen;
		MainPlayerController->ToggleInventory(bInventoryOpen);
		if (bInventoryOpen)
		{
			MainPlayerController->RemovePickupPrompt();
			SwitchMovementStyle(EMovementStyle::EMS_Free);
			SwitchCameraMovement(ECameraMovement::EMC_Inventory);

			CameraFixedRotation = FRotator(0.f, GetControlRotation().Yaw, 0.f);
			SpringArm->SetWorldRotation(CameraFixedRotation);

			DynamicMulticastOpenCharacterMenu.Broadcast();
		}
		else
		{
			SwitchCameraMovement(ECameraMovement::ECM_Free);
			DynamicMulticastResetInventoryHUD.Broadcast();
		}

		if (bLockedOn)
		{
			UntargetEnemy();
		}
	}
}

void AMainCharacter::Interact(const FInputActionValue& value)
{
	if (bCanPickup && !bInventoryOpen)
	{
		if (currentPickup)
		{
			switch (currentPickup->PickupType)
			{
			case EPickupType::EPT_RESOURCE:
			{
				mInventory->AddToResourceInventory(currentPickup->itemDataTable);
				break;
			}
			case EPickupType::EPT_WEAPON:
			{
				mInventory->AddToWeaponInventory(currentPickup->itemDataTable);
				break;
			}
			}

			currentPickup->Destroy();
			//DynamicMulticastSetImageAndCount.Broadcast();
			DynamicMulticastPickupItem.Broadcast();

			if (OverlappingPickups.Num() == 0)
			{
				MainPlayerController->RemovePickupPrompt();
			}
		}

		/*if (currentPickup->BPPickupItem)
		{
			TSubclassOf<AWeapon> PickupWeapon = currentPickup->BPPickupItem;
			if (PickupWeapon && mInventory)
			{
				mInventory->AddToWeaponInventory(PickupWeapon);
				if (mInventory->weaponInventory.Num() == 1) EquippedWeaponIndex = 0;

				currentPickup->Destroy();
				DynamicMulticastSetImageAndCount.Broadcast();
			}

			TSubclassOf<AResource> PickupResource = currentPickup->BPPickupItem;
			if (PickupResource)
			{
				AResource* resource = Cast<AResource>(PickupResource->GetDefaultObject());
				if (resource && mInventory)
				{
					resource->BuildResource(currentPickup->ItemName);
					FString Name = resource->resourceName;
					//mInventory->AddToResourceInventory(resource);

					UE_LOG(LogTemp, Warning, TEXT("pickup"));

					mInventory->AddToResourceInventory(currentPickup->itemDataTable);

					currentPickup->Destroy();
					DynamicMulticastSetImageAndCount.Broadcast();
				}
			}

			if (OverlappingPickups.Num() == 0)
			{
				MainPlayerController->RemovePickupPrompt();
			}
		}*/
	}
}

void AMainCharacter::Attack(const FInputActionValue& value)
{
	if (bAttacking || bBlocking || bDodging || bStunned || bInventoryOpen || rightHandEquipment == nullptr) return;
	if (mAttributes)
	{
		if (mAttributes->stamina < 0.0f) return;
	}

	bAttacking = true;

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && AttackMontage)
	{
		FString SectionName = FString::Printf(TEXT("Attack%d"), ComboCount);
		AnimInstance->Montage_Play(AttackMontage);
		if (bCanCombo)
		{
			AnimInstance->Montage_JumpToSection(*SectionName, AttackMontage);
		}
		else
		{
			AnimInstance->Montage_JumpToSection(*SectionName, AttackMontage);
		}

		if(mAttributes)
		{
			mAttributes->SetStaminaRechargeTimer(rightHandEquipment->staminaRechargeDelay);
			mAttributes->UseStamina(rightHandEquipment->staminaCost);
			mAttributes->ResetPoiseRecharge();
		}
	}
}

void AMainCharacter::StartBlock(const FInputActionValue& value)
{
	if (!bAttacking && !bDodging && !bInventoryOpen && rightHandEquipment)
	{
		bBlocking = true;
		if (rightHandEquipment)
		{
			rightHandEquipment->ActivateGuardCollision();
		}
	}
}

void AMainCharacter::EndBlock(const FInputActionValue& value)
{
	bBlocking = false;
	if (rightHandEquipment)
	{
		rightHandEquipment->DeactivateGuardCollision();
	}
}

void AMainCharacter::Dodge(const FInputActionValue& value)
{
	if (bDodging || bAttacking || bBlocking || bInventoryOpen) return;
	if (mAttributes)
	{
		if (mAttributes->stamina <= 0) return;
		
		mAttributes->UseStamina(25.0f);
		mAttributes->SetStaminaRechargeTimer(1.0f);
	}

	bDodging = true;
	ComboCount = 1;
	if (TargetEnemy)
	{
		if (mInputVector == FVector::ZeroVector)
		{
			UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
			if (AnimInstance)
			{
				AnimInstance->Montage_Play(DodgeMontage);
				AnimInstance->Montage_JumpToSection("Dodgeback");
			}

			return;
		}

		mInputVector.Normalize();
		FVector DesiredDirection = mInputVector.RotateAngleAxis(GetControlRotation().Yaw, FVector::UpVector);

		FVector RightDirection = FVector::CrossProduct(GetActorUpVector(), DesiredDirection);
		FVector ToEnemy = TargetEnemy->GetActorLocation() - GetActorLocation();
		ToEnemy.Normalize();
		FVector RelativeRight = FVector::CrossProduct(GetActorUpVector(), ToEnemy);

		float ForwardDot = FMath::RoundToFloat(FVector::DotProduct(GetActorForwardVector(), DesiredDirection));
		float RightDot = FMath::RoundToFloat(FVector::DotProduct(RelativeRight, DesiredDirection));

		UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
		if (AnimInstance && DodgeMontage)
		{
			if (ForwardDot == 1)
			{
				AnimInstance->Montage_Play(DodgeMontage);
				AnimInstance->Montage_JumpToSection("DodgeForward");
			}
			else if (RightDot  == 1)
			{
				AnimInstance->Montage_Play(DodgeMontage);
				AnimInstance->Montage_JumpToSection("DodgeRight");
			}
			else if (ForwardDot == -1)
			{
				AnimInstance->Montage_Play(DodgeMontage);
				AnimInstance->Montage_JumpToSection("DodgeBack");
			}
			else if (RightDot == -1)
			{
				AnimInstance->Montage_Play(DodgeMontage);
				AnimInstance->Montage_JumpToSection("DodgeLeft");
			}
		}
	}
	else
	{
		if (mInputVector.SizeSquared() > 0)
		{
			mInputVector.Normalize();
			FVector DesiredDirection = mInputVector.RotateAngleAxis(GetControlRotation().Yaw, FVector::UpVector);
			SetActorRotation(DesiredDirection.Rotation());
		}

		UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
		if (AnimInstance)
		{
			if (mInputVector.SizeSquared() == 0)
			{
				AnimInstance->Montage_Play(DodgeMontage);
				AnimInstance->Montage_JumpToSection("Dodgeback");
			}
			else
			{
				AnimInstance->Montage_Play(DodgeMontage);
				AnimInstance->Montage_JumpToSection("DodgeFront");
			}
		}
	}
}

void AMainCharacter::BlockHit()
{
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && GuardMontage)
	{
		AnimInstance->Montage_Play(GuardMontage);
		AnimInstance->Montage_JumpToSection("Guard_Hit");

		if(mAttributes)
		{
			mAttributes->UseStamina(15.0f);
			mAttributes->ResetStaminaRecharge();
		}
	}
}

void AMainCharacter::ToggleLockOn(const FInputActionValue& value)
{
	if (bInventoryOpen) return;

	bool LockedOn = FindBestTargetEnemy();
	if (!LockedOn)
	{
		UntargetEnemy();
		SwitchCameraMovement(ECameraMovement::ECM_Free);
	}

	SwitchMovementStyle(EMovementStyle::EMS_Free);
}

void AMainCharacter::OnePressed(const FInputActionValue& value)
{
	if (mInventory)
	{
		if (mInventory->gearSlotOneMap.Num() > 0 && Unoccupied())
		{
			UseGear(0);
			mDesiredGearSlot = 0;
		}
	}

	/*if (GearSlotOneInventory.Num() > 0 && !bInventoryOpen && !bUsing && !bDodging && !bAttacking && !bStunned && !bBlocking)
	{
		GetGear(0);
	}*/
}

void AMainCharacter::TwoPressed(const FInputActionValue& value)
{
	/*if (GearSlotTwoInventory.Num() > 0 && !bInventoryOpen && !bUsing && !bDodging && !bAttacking && !bStunned)
	{
		UseGear(1);
	}*/
	if (mInventory)
	{
		if (mInventory->gearSlotTwoMap.Num() > 0 && Unoccupied())
		{
			UseGear(1);
			mDesiredGearSlot = 1;
		}
	}
}

void AMainCharacter::ThreePressed(const FInputActionValue& value)
{
	/*if (GearSlotThreeInventory.Num() > 0 && !bInventoryOpen && !bUsing && !bDodging && !bAttacking && !bStunned)
	{
		UseGear(2);
	}*/

	if (mInventory)
	{
		if (mInventory->gearSlotThreeMap.Num() > 0 && Unoccupied())
		{
			UseGear(2);
			mDesiredGearSlot = 2;
		}
	}
}

void AMainCharacter::FourPressed(const FInputActionValue& value)
{
	/*if (GearSlotFourInventory.Num() > 0 && !bInventoryOpen && !bUsing && !bDodging && !bAttacking && !bStunned)
	{
		UseGear(3);
	}*/

	if (mInventory)
	{
		if (mInventory->gearSlotFourMap.Num() > 0 && Unoccupied())
		{
			UseGear(3);
			mDesiredGearSlot = 3;
		}
	}
}

void AMainCharacter::EnemyDetectionSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	/*if (OtherActor && OtherActor != this)
	{
		APawn* Pawn = Cast<APawn>(OtherActor);
		if (Pawn)
		{
			UE_LOG(LogTemp, Warning, TEXT("Enemy Detected: %s"), *OtherActor->GetName());
		}
	}*/
}

void AMainCharacter::EnemyDetectionSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	/*if (OtherActor && OtherActor != this)
	{
		APawn* Pawn = Cast<APawn>(OtherActor);
		if (Pawn)
		{
			UE_LOG(LogTemp, Warning, TEXT("Enemy Lost: %s"), *OtherActor->GetName());
		}
	}*/
}

void AMainCharacter::CheckOverlappingPickups()
{
	if (OverlappingPickups.Num() > 0)
	{
		float BestPickup = 100000.f;
		for (APickup* Pickup : OverlappingPickups)
		{
			float SqrDistance = FVector::DistSquared(GetActorLocation(), Pickup->GetActorLocation());
			if (SqrDistance < BestPickup)
			{
				currentPickup = Pickup;
				BestPickup = SqrDistance;
			}
		}

		if (currentPickup)
		{
			MainPlayerController->DisplayPickupPrompt(currentPickup->GetPickupPrompt());
		}
	}
	else if (OverlappingPickups.Num() == 1)
	{
		currentPickup = OverlappingPickups[0];
		if (currentPickup)
		{
			MainPlayerController->DisplayPickupPrompt(currentPickup->GetPickupPrompt());
		}
	}
}

void AMainCharacter::SetPickups(APickup* Pickup, bool Overlapped)
{
	if (Overlapped)
	{
		PickupsCount++;
		bCanPickup = PickupsCount > 0;
		OverlappingPickups.Add(Pickup);
	}
	else
	{
		PickupsCount--;
		bCanPickup = PickupsCount > 0;
		OverlappingPickups.Remove(Pickup);

		if (PickupsCount == 0)
		{
			MainPlayerController->RemovePickupPrompt();
		}
	}
}

bool AMainCharacter::FindBestTargetEnemy()
{
	if (bLockedOn) return false;

	float BestEnemyValue = 100000.f;

	TArray<AActor*> OverlappedEnemies;
	EnemyDetectionSphere->GetOverlappingActors(OverlappedEnemies, AEnemy::StaticClass());
	if (OverlappedEnemies.Num() > 0)
	{
		for (AActor* Enemy : OverlappedEnemies)
		{
			if (Enemy == this) continue;

			//Can Camera and Character can see Enemy
			FHitResult HitResult;
			FVector CameraStart = Camera->GetComponentLocation();
			FVector End = Enemy->GetActorLocation();
			GetWorld()->LineTraceSingleByChannel(HitResult, CameraStart, End, ECollisionChannel::ECC_Visibility);
			if (HitResult.bBlockingHit)
			{
				continue;
			}
			else
			{
				FVector CharacterStart = GetActorLocation();
				GetWorld()->LineTraceSingleByChannel(HitResult, CharacterStart, End, ECollisionChannel::ECC_Visibility);
				if(HitResult.bBlockingHit) continue;
			}

			//Is Enemy in range
			FVector EnemyVector = Enemy->GetActorLocation() - GetActorLocation();
			float Distance = EnemyVector.Size();
			if (Distance > MaxLockOnDistance) continue;
			else
			{
				Distance *= 0.5f;
			}

			FVector2D ScreenSize;
			if (GEngine && GEngine->GameViewport)
			{
				GEngine->GameViewport->GetViewportSize(ScreenSize);
			}

			FVector2D ScreenCenter(ScreenSize.X * 0.5f, ScreenSize.Y * 0.5f);
			FVector2D EnemyScreenPosition;
			bool bEnemyScreenToWorld = UGameplayStatics::ProjectWorldToScreen(UGameplayStatics::GetPlayerController(this, 0), Enemy->GetActorLocation(), EnemyScreenPosition);

			FVector2D EnemyScreenVector = EnemyScreenPosition - ScreenCenter;
			float ScreenCenterDistance = EnemyScreenVector.Size();

			if (ScreenCenterDistance + Distance < BestEnemyValue)
			{
				BestEnemyValue = ScreenCenterDistance + Distance;
				TargetEnemy = Cast<AEnemy>(Enemy);
			}
		}

		if (TargetEnemy != nullptr)
		{
			bLockedOn = true;
			TargetEnemy->ShowHealthBar(true);
			return true;
		}
	}

	return false;
}

void AMainCharacter::LockOnEnemy()
{
	if (MainPlayerController && TargetEnemy)
	{
		if (TargetEnemy->LockOnPosition)
		{
			MainPlayerController->DisplayLockOn(TargetEnemy->LockOnPosition->GetComponentLocation());
		}
	}
}

void AMainCharacter::UntargetEnemy()
{
	if (MainPlayerController && TargetEnemy)
	{
		TargetEnemy->HideHealthBar();
		TargetEnemy = nullptr;
		MainPlayerController->RemoveLockOn();
		bLockedOn = false;
	}
}

void AMainCharacter::ResetState()
{
	bAttacking = bDodging = bStunned = bUsing = false;
}

void AMainCharacter::SwitchMovementStyle(EMovementStyle MovementStyle)
{
	switch (MovementStyle)
	{
	case EMovementStyle::EMS_Free:
		bUseControllerRotationPitch = false;
		bUseControllerRotationYaw = false;
		bUseControllerRotationRoll = false;

		GetCharacterMovement()->bOrientRotationToMovement = true;
		GetCharacterMovement()->RotationRate = FRotator(0.f, 500.f, 0.f);
		break;
	case EMovementStyle::EMS_LockedOn:
		bUseControllerRotationYaw = false;
		GetCharacterMovement()->bOrientRotationToMovement = false;
		break;
	default:
		break;
	}
}

void AMainCharacter::SwitchCameraMovement(ECameraMovement CameraMovement)
{
	switch (CameraMovement)
	{
	case ECameraMovement::ECM_Free:
		SpringArm->SetWorldLocation(RootComponent->GetComponentLocation() + FVector(0.f, 0.f, 60.f));
		SpringArm->TargetArmLength = SpringArmDefaultLength;
		SpringArm->bUsePawnControlRotation = true;
		break;
	case ECameraMovement::EMC_LockedOn:
		break;
	case ECameraMovement::EMC_Inventory:
		SpringArm->TargetArmLength = 250.f;
		SpringArm->bUsePawnControlRotation = false;
		break;
	default:
		break;
	}
}

void AMainCharacter::LookAtTarget()
{
	//Rotate Actor to face Enemy
	if (bFaceTarget)
	{
		FVector LookVector = TargetEnemy->GetActorLocation() - GetActorLocation();
		LookVector.Z = 0.f;
		FRotator LookRotation = LookVector.Rotation();

		SetActorRotation(LookRotation);
	}

	//Set SprinArm location between Player and Enemy
	FVector CombatCenter = GetActorLocation() + (TargetEnemy->LockOnPosition->GetComponentLocation() - GetActorLocation()) * 0.5;
	SpringArm->SetWorldLocation(CombatCenter);

	//Adjust SpringArm length based on distance 
	float CombatDistance = FVector::Distance(GetActorLocation(), TargetEnemy->GetActorLocation());
	CombatDistance = FMath::Clamp(CombatDistance, 500.f, MaxLockOnDistance);
	SpringArm->TargetArmLength = CombatDistance;
}

void AMainCharacter::LookAtInventory()
{
	//FRotator CameraRotation{ 0.f, GetControlRotation().Yaw, 0.f };
	SpringArm->SetWorldRotation(CameraFixedRotation);

	FVector LeftDirection = FVector::CrossProduct(Camera->GetForwardVector(), FVector::UpVector) * 100.f;
	SpringArm->SetWorldLocation(GetActorLocation() + LeftDirection);
}

void AMainCharacter::EquipWeaponR(int32 index)
{
	if (mInventory)
	{
		if (mInventory->weaponMap.Num() > 0 && index <= mInventory->weaponMap.Num() - 1)
		{
			EquipRightHand(mInventory->weaponMap[index].weaponBlueprint);
			if (rightHandEquipment)
			{
				rightHandEquipment->weaponType = mInventory->weaponMap[index].weaponType;
				rightHandEquipment->weaponImage = mInventory->weaponMap[index].weaponImage;
				rightHandEquipment->damage = mInventory->weaponMap[index].damage;
				rightHandEquipment->staminaCost = mInventory->weaponMap[index].staminaCost;
				rightHandEquipment->staminaRechargeDelay = mInventory->weaponMap[index].staminaRechargeDelay;
				rightHandEquipment->poiseCost = mInventory->weaponMap[index].poiseCost;
			}
		}
	}
}

void AMainCharacter::EquipWeaponL(int32 index)
{
	
}

void AMainCharacter::UnequipWeaponR()
{
	if (rightHandEquipment)
	{
		rightHandEquipment->Destroy();
		rightHandEquipment = nullptr;
	}
}

UTexture2D* AMainCharacter::GetRightWeaponImage()
{
	if (rightHandEquipment)
	{
		return rightHandEquipment->weaponImage;
	}

	return nullptr;
}

void AMainCharacter::UseGear(int32 gearSlotIndex)
{
	if (mInventory)
	{
		switch (gearSlotIndex)
		{
		case 0:
		{
			if (mInventory->gearSlotOneMap.begin())
			{
				PlayUseMontage(FName(mInventory->gearSlotOneMap.begin().Key().montageSection));
			}
			break;
		}
		case 1:
		{
			if (mInventory->gearSlotTwoMap.begin())
			{
				PlayUseMontage(FName(mInventory->gearSlotTwoMap.begin().Key().montageSection));
			}
			break;
		}
		case 2:
		{
			if (mInventory->gearSlotThreeMap.begin())
			{
				PlayUseMontage(FName(mInventory->gearSlotThreeMap.begin().Key().montageSection));
			}
			break;
		}
		case 3:
		{
			if (mInventory->gearSlotFourMap.begin())
			{
				PlayUseMontage(FName(mInventory->gearSlotFourMap.begin().Key().montageSection));
			}
			break;
		}
		}
	}
}

void AMainCharacter::UseGearFromInventory()
{
	if (mDesiredGearSlot >= 0 && mInventory)
	{
		mStatusEffects->AddStatusEffect(mInventory->GetGearStatusEffect(mDesiredGearSlot), this);
		mInventory->RemoveFromGearSlot(mDesiredGearSlot, 1);
		DynamicMulticastUseGear.Broadcast();
	}
}

void AMainCharacter::Heal(float Amount)
{
	if(mAttributes)
	{
		mAttributes->health += Amount;
		if (mAttributes->health > mAttributes->maxHealth) mAttributes->health = mAttributes->maxHealth;
	}
}

void AMainCharacter::PlayUseMontage(FName MontageSection)
{
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance)
	{
		bUsing = true;
		AnimInstance->Montage_Play(UseMontage);
		AnimInstance->Montage_JumpToSection(MontageSection);
	}
}

void AMainCharacter::Stagger()
{
	bStunned = true;
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && HurtMontage)
	{
		AnimInstance->Montage_Play(HurtMontage);
		AnimInstance->Montage_JumpToSection(TEXT("HurtFront"));
	}

	bAttacking = false;

	bUsing = false;
	bDodging = false;
	ResetCombo();
}

void AMainCharacter::Recoil()
{
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && RecoilMontage)
	{
		AnimInstance->Montage_Play(RecoilMontage);
		AnimInstance->Montage_JumpToSection("");
	}
}

void AMainCharacter::ResetStunned()
{
	bStunned = false;
}

bool AMainCharacter::Unoccupied()
{
	return !bInventoryOpen && !bUsing && !bDodging && !bAttacking && !bStunned;
}

void AMainCharacter::ResetDodge()
{
	bAttacking = false;
	bDodging = false;
	bCanDodge = true;
}

void AMainCharacter::OpenCombo()
{
	bAttacking = false;
	bCanCombo = true;
	ComboCount++;
}

void AMainCharacter::ResetCombo()
{
	bAttacking = false;
	bCanCombo = false;
	ComboCount = 1;
}

void AMainCharacter::Die(AActor* Causer)
{
	AEnemy* Enemy = Cast<AEnemy>(this);
	if (Enemy)
	{
		Enemy->SetDead();
	}

	AWeapon* Weapon = Cast<AWeapon>(Causer);
	if (Weapon)
	{
		AMainCharacter* MainCharacter = Cast<AMainCharacter>(Weapon->GetCharacter());
		if (MainCharacter)
		{
			MainCharacter->UntargetEnemy();
			MainCharacter->SwitchMovementStyle(EMovementStyle::EMS_Free);
			MainCharacter->SwitchCameraMovement(ECameraMovement::ECM_Free);
		}
	}

	UAnimInstance* AnimInstace = GetMesh()->GetAnimInstance();
	if (AnimInstace && DeathMontage)
	{
		AnimInstace->Montage_Play(DeathMontage, 1.0);
		AnimInstace->Montage_JumpToSection(FName("Death"), DeathMontage);
	}

	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	bAttacking = false;
}

void AMainCharacter::DeathEnd()
{
	GetMesh()->bPauseAnims = true;
	Destroy();

	UKismetSystemLibrary::QuitGame(this, Cast<APlayerController>(GetController()), EQuitPreference::Quit, true);
}

void AMainCharacter::EquipRightHand(TSubclassOf<AWeapon> weaponToEquip)
{
	if (weaponToEquip)
	{
		if (rightHandEquipment != nullptr)
		{
			rightHandEquipment->Destroy();
		}

		AActor* weapon = GetWorld()->SpawnActor(weaponToEquip);
		const USkeletalMeshSocket* rightHandSocket = GetMesh()->GetSocketByName("Weapon_R");
		if (rightHandSocket)
		{
			rightHandSocket->AttachActor(weapon, GetMesh());
		}

		rightHandEquipment = Cast<AWeapon>(weapon);
		if (rightHandEquipment)
		{
			rightHandEquipment->SetMainCharacter(this);
		}
	}
	else if (weaponToEquip == nullptr)
	{
		if (rightHandEquipment != nullptr)
		{
			rightHandEquipment->Destroy();
		}
	}
}

void AMainCharacter::EquipLeftHand(TSubclassOf<AWeapon> weaponToEquip)
{
	if (weaponToEquip)
	{
		if (leftHandEquipment != nullptr)
		{
			leftHandEquipment->Destroy();
		}

		AActor* weapon = GetWorld()->SpawnActor(weaponToEquip);
		const USkeletalMeshSocket* leftHandSocket = GetMesh()->GetSocketByName("Weapon_L");
		if (leftHandSocket)
		{
			leftHandSocket->AttachActor(weapon, GetMesh());
		}

		leftHandEquipment = Cast<AWeapon>(weapon);
		if (leftHandEquipment)
		{
			leftHandEquipment->SetMainCharacter(this);
		}
	}
	else if (weaponToEquip == nullptr)
	{
		if (leftHandEquipment != nullptr)
		{
			leftHandEquipment->Destroy();
		}
	}
}

void AMainCharacter::ActivateWeapon()
{
	if (rightHandEquipment)
	{
		rightHandEquipment->ActivateAttackCollision();
	}
}

void AMainCharacter::DeactivateWeapon()
{
	if (rightHandEquipment)
	{
		rightHandEquipment->DeactivateAttackCollision();
	}
}
