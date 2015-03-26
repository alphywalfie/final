#include "boxCollision.hpp"

bool collisionDetectedBoxCircle(SDL_Rect box, double degrees, SDL_Rect circle, double radius)
{
	float theta1 = degrees/(float)180*PI;
	double c1x = box.x+box.w/2, c1y = box.y+box.h/2,
		c2x = circle.x+radius, c2y = circle.y+radius;
	vec2d hw, hh;
	
	double cs = cos(theta1);
	double sn = sin(theta1);
	hw.x = (box.w/2)*cs;
	hw.y = (box.w/2)*sn;
	hh.x = -(box.h/2)*sn;
	hh.y = (box.h/2)*cs;
	
	vec2d centerVec;
	centerVec.x = c2x - c1x;
	centerVec.y = c2y - c1y;
	//three axes of separation

	//centers OK
	double distSqrd = (centerVec.x*centerVec.x)+(centerVec.y*centerVec.y);
	vec2d projAxis = normalize2d(centerVec);	
	double temp1 = findTheMax(hh,hw,projAxis);
	if (distSqrd > (temp1+radius)*(temp1+radius))
	{
		return false;
	}

	////hw
	projAxis = normalize2d(hw);
	double dist = dotProduct2D(centerVec, projAxis);
	temp1 = findTheMax(hh,hw,projAxis);
	if (dist*dist > (temp1+radius)*(temp1+radius))
	{
		return false;
	}

	////hh OK
	projAxis = normalize2d(hh);
	dist = dotProduct2D(centerVec, projAxis);
	temp1 = findTheMax(hh,hw,projAxis);
	if (dist*dist > (temp1+radius)*(temp1+radius))
	{
		return false;
	}

	return true;
}









