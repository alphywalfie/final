struct vec2d {double x,  y;};

double dotProduct2D(vec2d vec1, vec2d vec2)
{
	return ((vec1.x*vec2.x)+(vec1.y*vec2.y));
}

double magnitude2d(vec2d vec)
{
	return sqrt(vec.x*vec.x+vec.y*vec.y);
}

vec2d normalize2d(vec2d vec)
{
	double mag = magnitude2d(vec);
	vec2d normalized;
	normalized.x = vec.x/mag;
	normalized.y = vec.y/mag;
	return normalized;
}

double findTheMax(vec2d hh, vec2d hw, vec2d projAxis)
{
	double result;
	double temp;

	//++
	result = dotProduct2D(hh,projAxis)+dotProduct2D(hw,projAxis);

	//--
	temp = -dotProduct2D(hh,projAxis)-dotProduct2D(hw,projAxis);
	if (temp > result)
	{
		result = temp;
	}

	//+-
	temp = dotProduct2D(hh,projAxis)-dotProduct2D(hw,projAxis);
	if (temp > result)
	{
		result = temp;
	}

	//-+
	temp = -dotProduct2D(hh,projAxis)+dotProduct2D(hw,projAxis);
	if (temp > result)
	{
		result = temp;
	}

	return result;
}
 
bool collisionDetected(SDL_Rect box1, double degrees1, SDL_Rect box2, double degrees2)
{
	float theta1 = degrees1/(float)180*PI;
	float theta2 = degrees2/(float)180*PI;
	double c1x = box1.x+box1.w/2, c1y = box1.y+box1.h/2,
		c2x = box2.x+box2.w/2, c2y = box2.y+box2.h/2;
	vec2d hw1, hh1, hw2, hh2;

	double cs = cos(theta1);
	double sn = sin(theta1);
	hw1.x = (box1.w/2)*cs;
	hw1.y = (box1.w/2)*sn;
	hh1.x = -(box1.h/2)*sn;
	hh1.y = (box1.h/2)*cs;

	cs = cos(theta2);
	sn = sin(theta2);
	hw2.x = (box2.w/2)*cs;
	hw2.y = (box2.w/2)*sn;
	hh2.x = -(box2.h/2)*sn;
	hh2.y = (box2.h/2)*cs;
	
	vec2d centerVec;
	centerVec.x = c2x - c1x;
	centerVec.y = c2y - c1y;
	//something tells me mali 'to
	vec2d centerVecPerp;
	centerVecPerp.x = -centerVec.y;
	centerVecPerp.y = centerVec.x;
	
	//six axes of separation

	//centers OK
	double distSqrd = (centerVec.x*centerVec.x)+(centerVec.y*centerVec.y);
	vec2d projAxis = normalize2d(centerVec);	
	double temp1 = findTheMax(hh1,hw1,projAxis);
	double temp2 = findTheMax(hh2,hw2,projAxis);
	if (distSqrd > (temp1+temp2)*(temp1+temp2))
	{
		return false;
	}


	////hw1
	projAxis = normalize2d(hw1);
	double dist = dotProduct2D(centerVec, projAxis);
	temp1 = findTheMax(hh1,hw1,projAxis);
	temp2 = findTheMax(hh2,hw2,projAxis);
	if (dist*dist > (temp1+temp2)*(temp1+temp2))
	{
		return false;
	}

	////hh1 OK
	projAxis = normalize2d(hh1);
	dist = dotProduct2D(centerVec, projAxis);
	temp1 = findTheMax(hh1,hw1,projAxis);
	temp2 = findTheMax(hh2,hw2,projAxis);
	if (dist*dist > (temp1+temp2)*(temp1+temp2))
	{
		return false;
	}

	////hw2 OK
	projAxis = normalize2d(hw2);
	dist = dotProduct2D(centerVec, projAxis);
	temp1 = findTheMax(hh1,hw1,projAxis);
	temp2 = findTheMax(hh2,hw2,projAxis);
	if (dist*dist > (temp1+temp2)*(temp1+temp2))
	{
		return false;
	}

	////hh2 OK
	projAxis = normalize2d(hh2);
	dist = dotProduct2D(centerVec, projAxis);
	temp1 = findTheMax(hh1,hw1,projAxis);
	temp2 = findTheMax(hh2,hw2,projAxis);
	if (dist*dist > (temp1+temp2)*(temp1+temp2))
	{
		return false;
	}

	return true;
}

bool collisionDetectedAABB(SDL_Rect box1, SDL_Rect box2)
{
	//box 1 as reference
	if (box2.x < box1.x && box1.x < box2.x+box2.w &&  box2.y < box1.y && box1.y < box2.y+box2.h)
	{
		return true;
	}
	if (box2.x < box1.x+box1.w && box1.x+box1.w < box2.x+box2.w &&  box2.y < box1.y && box1.y < box2.y+box2.h)
	{
		return true;
	}
	if (box2.x < box1.x && box1.x < box2.x+box2.w &&  box2.y < box1.y+box1.h && box1.y+box1.h < box2.y+box2.h)
	{
		return true;
	}
	if (box2.x < box1.x+box1.w && box1.x+box1.w < box2.x+box2.w &&  box2.y < box1.y+box1.h && box1.y+box1.h < box2.y+box2.h)
	{
		return true;
	}

	//box 2 as reference
	if (box1.x < box2.x && box2.x < box1.x+box1.w &&  box1.y < box2.y && box2.y < box1.y+box1.h)
	{
		return true;
	}
	if (box1.x < box2.x+box2.w && box2.x+box2.w < box1.x+box1.w &&  box1.y < box2.y && box2.y < box1.y+box1.h)
	{
		return true;
	}
	if (box1.x < box2.x && box2.x < box1.x+box1.w &&  box1.y < box2.y+box2.h && box2.y+box2.h < box1.y+box1.h)
	{
		return true;
	}
	if (box1.x < box2.x+box2.w && box2.x+box2.w < box1.x+box1.w &&  box1.y < box2.y+box2.h && box2.y+box2.h < box1.y+box1.h)
	{
		return true;
	}

	/*if ((box2.x <= box1.x))
	{

	}*/
	return false;
}









