#include "stdafx.h"
#include "math.h"

#include <SDL2\sdl.h>

#include <iostream>
#include <vector>

std::vector<SDL_Joystick *> JOYSTICK;
unsigned buttonLimiter[20];
unsigned buttonHitter[20];

bool init_joystick()
{
	if (SDL_InitSubSystem(SDL_INIT_JOYSTICK) < 0)
	{
		std::cout << "Unable to initialize Joystick:\t" << SDL_GetError() << std::endl;
		return false;
	}

	int nj = SDL_NumJoysticks();
	for (int i = 0; i < nj;i++)
	{
		SDL_Joystick *joy = SDL_JoystickOpen(i);
		if (joy)
		{
			if (!strncmp("Saitek", SDL_JoystickName(joy), 6))
				SDL_JoystickClose(joy);
			else JOYSTICK.push_back(joy);
		}
	}

	if (JOYSTICK.empty())
	{
		std::cout << "No Joystick Found!" << std::endl;
		return false;
	}

	SDL_JoystickEventState(SDL_ENABLE);

	memset(buttonLimiter, 0, sizeof(buttonLimiter));
	memset(buttonHitter, 0, sizeof(buttonHitter));

	return true;
}

bool poll_joystick(int &x, int &y, int &b)
{
	if (JOYSTICK.empty())
	{
		return false;
	}

	int i;
	unsigned int n;
	signed short a;

	SDL_JoystickUpdate();

	for (i = 0; i < SDL_JoystickNumButtons(JOYSTICK.front());i++)
	{
		n = SDL_JoystickGetButton(JOYSTICK.front(), i);
//		if (!n)
//		{
//			b += int(std::pow(2.0f, i));
//		}
		if (n)
		{
			b = n;
		}
	}

	for (i = 0; i < SDL_JoystickNumAxes(JOYSTICK.front());i++)
	{
		a = SDL_JoystickGetAxis(JOYSTICK.front(), i);
		if (i == 0) x = a;
		if (i == 1) y = a;
	}

	return true;
}

void close_joystick()
{
	if (JOYSTICK.empty())
	{
		return;
	}

	std::vector<SDL_Joystick *>::iterator i = JOYSTICK.begin();
	while (i != JOYSTICK.end())
	{
		SDL_JoystickClose(*i);
		i++;
	}
}

unsigned getNumButtons()
{
	return SDL_JoystickNumButtons(JOYSTICK.front());
}