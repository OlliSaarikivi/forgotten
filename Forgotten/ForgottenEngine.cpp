#include "stdafx.h"
#include "ForgottenEngine.h"


ForgottenEngine::ForgottenEngine() :
monster_targeting{ monsters, players, monster_targetings },
monster_movement{ monster_targetings, monsters, monsters }
{
}
