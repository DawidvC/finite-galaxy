// Hardpoint.h

#ifndef HARDPOINT_H_
#define HARDPOINT_H_

#include "Angle.h"
#include "Point.h"

#include <vector>

class Outfit;
class Projectile;
class Ship;
class Visual;



// A single weapon hardpoint on the ship (i.e. a gun port or turret mount),
// which may or may not have a weapon installed.
class Hardpoint {
public:
  // Constructor. Hardpoints may or may not specify what weapon is in them.
  Hardpoint(const Point &point, bool isTurret, const Outfit *outfit = nullptr);

  // Get the weapon installed in this hardpoint (or null if there is none).
  const Outfit *GetOutfit() const;
  // Get the location, relative to the centre of the ship, from which
  // projectiles of this weapon should originate. This point must be
  // rotated to take the ship's current facing direction into account.
  const Point &GetPoint() const;
  // Get the angle that this weapon is aimed at, relative to the ship.
  const Angle &GetAngle() const;
  // Get the angle this weapon ought to point at for ideal gun harmonization.
  Angle HarmonizedAngle() const;
  // Shortcuts for querying weapon characteristics.
  bool IsTurret() const;
  bool IsHoming() const;
  bool IsAntiMissile() const;
  bool CanAim() const;

  // Check if this weapon is ready to fire.
  bool IsReady() const;
  // Check if this weapon was firing in the previous step.
  bool WasFiring() const;
  // If this is a burst weapon, get the number of shots left in the burst.
  int BurstRemaining() const;
  // Perform one step (i.e. decrement the reload count).
  void Step();

  // Adjust this weapon's aim by the given amount, relative to its maximum
  // "turret turn" rate.
  void Aim(double amount);
  // Fire this weapon. If it is a turret, it automatically points toward
  // the given ship's target. If the weapon requires ammunition, it will
  // be subtracted from the given ship.
  void Fire(Ship &ship, std::vector<Projectile> &projectiles, std::vector<Visual> &visuals);
  // Fire an anti-missile. Returns true if the missile should be killed.
  bool FireAntiMissile(Ship &ship, const Projectile &projectile, std::vector<Visual> &visuals);

  // Install a weapon here (assuming it is empty). This is only for
  // Armament to call internally.
  void Install(const Outfit *outfit);
  // Reload this weapon.
  void Reload();
  // Uninstall the outfit from this port (if it has one).
  void Uninstall();


private:
  // Reset the reload counters and expend ammunition, if any.
  void Fire(Ship &ship, const Point &start, const Angle &aim);


private:
  // The weapon installed in this hardpoint.
  const Outfit *outfit = nullptr;
  // Hardpoint location, in world coordinates relative to the ship's centre.
  Point point;
  // Angle adjustment for convergence.
  Angle angle;
  // Reload timers and other attributes.
  double reload = 0.;
  double burstReload = 0.;
  int burstCount = 0;
  bool isTurret = false;
  bool isFiring = false;
  bool wasFiring = false;
};



#endif
