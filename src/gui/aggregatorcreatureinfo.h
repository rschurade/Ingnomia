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
#pragma once

#include "../game/creature.h"

#include <QObject>

#include "../game/gnome.h"
#include "../game/militarymanager.h"

class Game;

struct GuiCreatureInfo
{
	QString name;
	unsigned int id = 0;
	QString profession;
	int str = 0;
	int dex = 0;
	int con = 0;
	int intel = 0;
	int wis = 0;
	int cha = 0;
	int hunger = 0;
	int thirst = 0;
	int sleep = 0;
	int happiness = 0;

	QString activity;

	Uniform uniform;
	Equipment equipment;

	QMap< QString, std::vector<unsigned char> > itemPics;
};
Q_DECLARE_METATYPE( GuiCreatureInfo )


class AggregatorCreatureInfo : public QObject
{
	Q_OBJECT

public:
	AggregatorCreatureInfo( QObject* parent = nullptr );

	void init( Game* game );

	void update();

private:
	QPointer<Game> g;

	GuiCreatureInfo m_info;
	QMap< QString, std::vector<unsigned char> > m_emptyPics;

	unsigned int m_currentID = 0;
	unsigned int m_previousID = 0;

	void createItemImg( QString slot, EquipmentItem& eItem );
	void createUniformImg( QString slot, const UniformItem& uItem, EquipmentItem& eItem );
	void createEmptyUniformImg( QString spriteID );

public slots:
	void onRequestCreatureUpdate( unsigned int creatureID );
	void onRequestProfessionList();
	void onSetProfession( unsigned int gnomeID, QString profession );

	void onRequestEmptySlotImages();

signals:
	void signalCreatureUpdate( const GuiCreatureInfo& info );
	void signalProfessionList( const QStringList& profs );
	
	void signalEmptyPics( const QMap< QString, std::vector<unsigned char> >& emptyPics );

};