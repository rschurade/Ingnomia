//
// Created by Arcnor on 25/07/2022.
// Copyright (c) 2022 Reversed Games. All rights reserved.
//

#ifndef INGNOMIA_DEBUGMANAGER_H
#define INGNOMIA_DEBUGMANAGER_H

// TODO: Actually exclude this file from compilation when building for debug
#ifdef _DEBUG

class DebugManager
{
public:
	void showDebug();

	void renderGeneralTab();
	void renderGnomeManagerTab();
};

#endif // _DEBUG

#endif // INGNOMIA_DEBUGMANAGER_H
