// Several useful C functions that can be used across many programs

// Putting this in a .h file is even worse!

// The correct way to do this would be to compile these into an object
// (.o) file and then compile my programs against that object file. I
// know this, but am too lazy (and dislike C too much) to do this

// TODO: these are wrong because I don't account for TDB-UTC (using
// deltet_c) like I should (but now fixing for unix conversions)

double et2jd(double d) {return 2451545.+d/86400.;}
double jd2et(double d) {return 86400.*(d-2451545.);}

// TODO: check routines below, I may have delta backwards or for wrong era

double unix2et(double d) {
  // compute delta
  SpiceDouble delta;
  deltet_c(d-946728000,"UTC",&delta);
  return d-946728000.+delta;
}

double et2unix(double d) {
  // compute delta
  SpiceDouble delta;
  deltet_c(d,"ET",&delta);
  return d+946728000.-delta;
}

// TODO: replace this with rpd_c or dpr_c everywhere it appears
double r2d(double d) {return d*180./pi_c();}

void posxyz(double time, int planet, SpiceDouble position[3]) {
  SpiceDouble lt;
  spkezp_c(planet, time,"J2000","NONE",0,position,&lt);
}

void earthvector(double time, int planet, SpiceDouble position[3]) {
  SpiceDouble lt;
  spkezp_c(planet, time,"J2000","NONE",399,position,&lt);
}

double earthangle(double time, int p1, int p2) {
  SpiceDouble pos[3], pos2[3];
  earthvector(time, p1, pos);
  earthvector(time, p2, pos2);
  return vsep_c(pos,pos2);
}

double earthmaxangle(double time, int arrsize, SpiceInt *planets) {
  double max=0, sep;

  int i,j;

  for (i=0; i<arrsize; i++) {
    if (planets[i]==0) {continue;}
    for (j=i+1; j<arrsize; j++) {
      if (planets[j]==0) {continue;}
      sep = earthangle(time, planets[i], planets[j]);
      if (sep>max) {max=sep;}
    }
  }
  return max;
}

// min angle from sun of given planets at given time

double sunminangle(double time, int arrsize, SpiceInt *planets) {

  // max angle is actually pi/2, so 3.1416 is overkill
  double min=3.1416, sep;
  int i;

  for (i=0; i<arrsize; i++) {
    if (planets[i]==0) {continue;}
    sep = earthangle(time, planets[i], 10);
    if (sep<min) {min=sep;}
  }
  return min;
}


// determine (sky) elevation of body at given time and place (radians
// and meters) on Earth

double bc_sky_elev (double latitude, double longitude, double elevation, double unixtime, char *target) {

  SpiceDouble radii[3], pos[3], normal[3], state[6], lt;
  SpiceInt n;
  
  // the Earth's equatorial and polar radii
  bodvrd_c("EARTH", "RADII", 3, &n, radii);

  // position of latitude/longitude/elevation on ellipsoid
  georec_c(longitude, latitude, elevation/1000., radii[0], 
	   (radii[0]-radii[2])/radii[0], pos);

  // surface normal vector to ellipsoid at latitude/longitude (this is
  // NOT the same as pos!)
  surfnm_c(radii[0], radii[1], radii[2], pos, normal);

  // find the position
  spkcpo_c(target, unix2et(unixtime), "ITRF93", "OBSERVER", "CN+S", pos, 
	   "Earth", "ITRF93", state,  &lt);

  // TODO: vsep_c below uses first 3 members of state, should I be
  // more careful here?

  return halfpi_c() - vsep_c(state,normal);
}


// for this functional version, angles are in radians, elevation in m
// stime, etime: start and end Unix times
// direction = "<" or ">", whether elevation above/below desire

SpiceDouble *bcriset (double latitude, double longitude, double elevation,
		double stime, double etime, char *target, double desired, 
		char *direction) {

  static SpiceDouble beg, end, results[10000];
  
  // TODO: compute this more efficiently, assuming no more than n
  // rises/day?
  SPICEDOUBLE_CELL(result, 10000);
  SPICEDOUBLE_CELL(cnfine,2);
  wninsd_c(stime, etime, &cnfine);

  // define gfq for geometry finder (nested functions ok per gcc)
  void gfq (SpiceDouble unixtime, SpiceDouble *value) {
    *value = bc_sky_elev(latitude, longitude, elevation, unixtime, target);
  }

  // TODO: this is silly and semi-pointless
  void gfdecrx (void(* udfuns)(SpiceDouble et,SpiceDouble * value),
		SpiceDouble et, SpiceBoolean * isdecr ) {
    SpiceDouble dt = 10.;
    uddc_c(udfuns, et, dt, isdecr);
  }
    
  // and now the geometry finder
  // TODO: is 3600 below excessive or too small?
  gfuds_c(gfq, gfdecrx, direction, desired, 0, 3600, 10000, &cnfine, &result);

  SpiceInt count = wncard_c(&result); 

  for (int i=0; i<count; i++) {
    wnfetd_c(&result,i,&beg,&end);
    results[i*2] = beg;
    results[i*2+1] = end;
  }

  return results;
}

// for this functional version, angles are in radians, elevation in m
// stime, etime: start and end Unix times
// direction = "<" or ">", whether elevation above/below desire

SpiceDouble *bc_between (double latitude, double longitude, double elevation,
			 double stime, double etime, char *target, double low,
			 double high) {

  static SpiceDouble beg, end, results[10000];
  
  // TODO: compute this more efficiently?
  SPICEDOUBLE_CELL(result, 10000);
  SPICEDOUBLE_CELL(cnfine,2);
  wninsd_c(stime, etime, &cnfine);

  // define gfq for geometry finder (nested functions ok per gcc)
  void gfq ( void (*udfuns) (SpiceDouble et, SpiceDouble  *value ),
	     SpiceDouble unixtime, SpiceBoolean * xbool ) {

    double elev=bc_sky_elev(latitude, longitude, elevation, unixtime, target);

    *xbool = (elev>=low && elev<=high);
  }

  // and now the geometry finder (assume condition met for at least 30s)
  gfudb_c(udf_c, gfq, 30, &cnfine, &result);

  SpiceInt count = wncard_c(&result); 

  for (int i=0; i<count; i++) {
    wnfetd_c(&result,i,&beg,&end);
    results[i*2] = beg;
    results[i*2+1] = end;
  }

  return results;
}
