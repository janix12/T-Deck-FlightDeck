#pragma once

static void updateMotion(Plane &p,const Plane &old){
  const long gap=p.observed-old.observed;
  if(gap<0 || gap>90) return;
  p.transitionUntil=old.transitionUntil;
  if(gap>0 && ((old.ground && p.airKnown) || (old.airKnown && p.ground)))
    p.transitionUntil=p.observed+60;
  if(!p.airKnown || !old.airKnown || !isfinite(p.heading) || !isfinite(old.heading) ||
      !isfinite(p.speed) || p.speed<100) return;
  float east=(p.lon-old.startLon)*111.2f*cosf(radians(p.lat));
  float north=(p.lat-old.startLat)*111.2f;
  if(!old.turnStart || gap>65 || p.observed-old.turnStart>900 ||
     p.observed-old.lastTurn>90 || fabsf(p.altitude-old.startAltitude)>350 ||
     east*east+north*north>18*18){
    p.turnStart=p.lastTurn=p.observed;
    p.startLat=p.lat; p.startLon=p.lon; p.startAltitude=p.altitude;
    return;
  }
  float change=remainderf(p.heading-old.heading,360.f);
  p.startLat=old.startLat; p.startLon=old.startLon; p.startAltitude=old.startAltitude;
  p.turnStart=old.turnStart; p.turnSum=old.turnSum+(gap?change:0);
  p.lastTurn=(gap && fabsf(change)>8)?p.observed:old.lastTurn;
}

static const char *flightState(const Plane &p){
  if(p.transitionUntil && p.observed<=p.transitionUntil){
    if(p.ground) return "Leszallt*";
    if(p.airKnown) return "Felszallt*";
  }
  if(p.ground){
    if(!isfinite(p.speed)) return "Foldon";
    return p.speed<3?"All a foldon*":"Foldon mozog*";
  }
  if(!p.airKnown) return "Nincs adat";
  if(p.turnStart && p.observed-p.turnStart>=180 && fabsf(p.turnSum)>=650) return "Korozes?*";
  if(!isfinite(p.verticalRate)) return "Levegoben";
  if(p.verticalRate>=300) return "Emelkedik*";
  if(p.verticalRate<=-300) return "Sullyed*";
  return "Szintrepules*";
}
