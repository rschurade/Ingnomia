/*	
	This file is part of Ingnomia https://github.com/rschurade/Ingnomia
    Copyright (C) 2017-2020  Ralph Schurade, Ingnomia Team

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as
    published by the Free Software Foundation, either version 3 of the
    License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
#ifndef __DebugModel_H__
#define __DebugModel_H__

#include "../aggregatordebug.h"

#include <QString>

#include <NsApp/DelegateCommand.h>
#include <NsApp/NotifyPropertyChangedBase.h>
#include <NsCore/Noesis.h>
#include <NsCore/Nullable.h>
#include <NsCore/Ptr.h>
#include <NsCore/ReflectionDeclare.h>
#include <NsCore/ReflectionDeclareEnum.h>
#include <NsCore/String.h>
#include <NsGui/Collection.h>

class DebugProxy;

namespace Noesis
{
template <class T>
class ObservableCollection;
}

namespace IngnomiaGUI
{

enum class DebugPage
{
	First,
	Second,
	Third
};
////////////////////////////////////////////////////////////////////////////////////////////////////
class DebugModel final : public NoesisApp::NotifyPropertyChangedBase
{
public:
	DebugModel();


private:
	DebugProxy* m_proxy = nullptr;

	DebugPage m_page = DebugPage::First;

	const char* GetShowFirst() const;
	const char* GetShowSecond() const;
	const char* GetShowThird() const;

	void onPageCmd( BaseComponent* param );
	const NoesisApp::DelegateCommand* GetPageCmd() const
	{
		return &m_pageCmd;
	}
	NoesisApp::DelegateCommand m_pageCmd;

	void onSpawnCmd( BaseComponent* param );
	const NoesisApp::DelegateCommand* GetSpawnCmd() const
	{
		return &m_spawnCmd;
	}
	NoesisApp::DelegateCommand m_spawnCmd;


	NS_DECLARE_REFLECTION( DebugModel, NotifyPropertyChangedBase )
};

} // namespace IngnomiaGUI

#endif
