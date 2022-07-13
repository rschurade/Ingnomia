#include "bt_factory.h"

#include <gtest/gtest.h>

#include <QXmlStreamReader>

const std::string contentSimple1 = R"(
<root main_tree_to_execute="MainTree">
	<BehaviorTree ID="MainTree">
		<FallbackStar name="AnimalStandard">
			<Action ID="RandomMoveBig"/>
		</FallbackStar>
	</BehaviorTree>
</root>
)";

const std::string contentSimple2 = R"(
<root main_tree_to_execute="ShedTree">
	<BehaviorTree ID="ShedTree">
		<FallbackStar>
			<SequenceStar>
				<Condition ID="IsOnPasture"/>
				<Condition ID="IsDay"/>
				<Condition ID="IsInShed"/>
				<Action ID="LeaveShed"/>
				<Action ID="FindRandomPastureField"/>
				<Action ID="Move"/>
			</SequenceStar>
			<SequenceStar>
				<Condition ID="IsOnPasture"/>
				<Condition ID="IsNight"/>
				<Inverter>
					<Condition ID="IsInShed"/>
				</Inverter>
				<Action ID="FindShed"/>
				<Action ID="Move"/>
				<Action ID="EnterShed"/>
			</SequenceStar>
			<SequenceStar>
				<Condition ID="IsDay"/>
				<Inverter>
					<Condition ID="IsInShed"/>
				</Inverter>
				<Action ID="RandomMove"/>
			</SequenceStar>
		</FallbackStar>
	</BehaviorTree>
</root>
)";

pugi::xml_document loadXml(const std::string& content) {
	pugi::xml_document result;
	result.load_buffer_inplace( (void*)content.c_str(), content.size() );
	return result;
}

TEST(SimpleTest1, BasicParsing) {
	BT_ActionMap result {
		{ "RandomMoveBig", [](bool){ return BT_RESULT::SUCCESS; } }
	};
	QVariantMap blackBoard;

	const auto xml = loadXml(contentSimple1);
	const auto root = xml.document_element();

	const auto* node = BT_Factory::load(root, result, blackBoard);
	EXPECT_EQ(node->status(), BT_RESULT::IDLE);
	EXPECT_NE(dynamic_cast<const BT_NodeFallbackStar*>( node ), nullptr);
}

TEST(SimpleTest2, BasicParsing) {
	BT_ActionMap result {
		{ "IsOnPasture", [](bool){ return BT_RESULT::SUCCESS; } },
		{ "IsDay", [](bool){ return BT_RESULT::SUCCESS; } },
		{ "IsInShed", [](bool){ return BT_RESULT::SUCCESS; } },
		{ "IsNight", [](bool){ return BT_RESULT::SUCCESS; } },

		{ "FindShed", [](bool){ return BT_RESULT::SUCCESS; } },
		{ "EnterShed", [](bool){ return BT_RESULT::SUCCESS; } },
		{ "LeaveShed", [](bool){ return BT_RESULT::SUCCESS; } },
		{ "FindRandomPastureField", [](bool){ return BT_RESULT::SUCCESS; } },
		{ "Move", [](bool){ return BT_RESULT::SUCCESS; } },
		{ "RandomMove", [](bool){ return BT_RESULT::SUCCESS; } },
	};
	QVariantMap blackBoard;

	const auto xml = loadXml(contentSimple2);
	const auto root = xml.document_element();

	const auto* node = BT_Factory::load(root, result, blackBoard);
	EXPECT_EQ(node->status(), BT_RESULT::IDLE);
	EXPECT_NE(dynamic_cast<const BT_NodeFallbackStar*>( node ), nullptr);
}