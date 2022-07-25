//
// Created by Arcnor on 25/07/2022.
// Copyright (c) 2022 Reversed Games. All rights reserved.
//

#ifndef INGNOMIA_DEBUGUTILS_H
#define INGNOMIA_DEBUGUTILS_H

#include "../base/position.h"

#include <QList>

#include <imgui.h>

namespace Debug
{
template <typename T>
void TabItemWithList( const std::string& label, QList<T*> list, int& id )
{
	if ( ImGui::BeginTabItem( label.c_str() ) )
	{
		const auto size = list.size();
		if ( size <= 0 )
		{
			ImGui::TextUnformatted( "No entries" );
		}
		else
		{
			ImGui::InputInt( "ID", &id );
			if ( id < 0 )
			{
				id = 0;
			}
			else if ( id >= size )
			{
				id = list.size() - 1;
			}

			if ( ImGui::BeginTabBar( "innerBar" ) )
			{
				list.at( id )->showDebug();
				ImGui::EndTabBar();
			}
		}

		ImGui::EndTabItem();
	}
}

static inline void Text( const char* fmt, ... )
{
	va_list args;
	va_start( args, fmt );
	ImGui::TextV( fmt, args );
	va_end( args );
}

static inline bool Input( const std::string& lbl, float* v )
{
	return ImGui::InputFloat( lbl.c_str(), v );
}

static inline bool Input( const std::string& lbl, char* v )
{
	return ImGui::InputScalar( lbl.c_str(), ImGuiDataType_S8, v );
}

static inline bool Input( const std::string& lbl, short* v )
{
	return ImGui::InputScalar( lbl.c_str(), ImGuiDataType_S16, v );
}

static inline bool Input( const std::string& lbl, int* v )
{
	return ImGui::InputInt( lbl.c_str(), v );
}

static inline bool Input( const std::string& lbl, int64_t* v )
{
	return ImGui::InputScalar( lbl.c_str(), ImGuiDataType_S64, v );
}

static inline bool Input( const std::string& lbl, unsigned char* v )
{
	return ImGui::InputScalar( lbl.c_str(), ImGuiDataType_U8, v );
}

static inline bool Input( const std::string& lbl, unsigned short* v )
{
	return ImGui::InputScalar( lbl.c_str(), ImGuiDataType_U16, v );
}

static inline bool Input( const std::string& lbl, unsigned int* v )
{
	return ImGui::InputScalar( lbl.c_str(), ImGuiDataType_U32, v );
}

static inline bool Input( const std::string& lbl, uint64_t* v )
{
	return ImGui::InputScalar( lbl.c_str(), ImGuiDataType_U64, v );
}

static inline bool Input( const std::string& lbl, bool* v )
{
	return ImGui::Checkbox( lbl.c_str(), v );
}

static inline bool Input( const std::string& lbl, Position* v )
{
	return ImGui::InputScalarN( lbl.c_str(), ImGuiDataType_S16, v, 3 );
}

static inline void Separator()
{
	ImGui::Separator();
}

static inline void Spacing()
{
	ImGui::Spacing();
}

} // namespace Debug

#endif // INGNOMIA_DEBUGUTILS_H
