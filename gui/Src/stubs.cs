using System.Windows.Controls;
using System.Collections.Generic;

namespace IngnomiaGUI
{
    public enum State
    {
        Main,
        Start,
        Settings,
        NewGame,
        LoadGame,
        Wait,
        GameRunning,
        Ingame
    };

    public partial class MainMenu : UserControl
    {
    }

    public partial class GameGui : UserControl
    { 
    }

    public class CommandButton
    {
        public string Name { get; set; }
    }

    public class BuildButton
    {
        public string Name { get; set; }
    }

    public class AvailableMaterial
    {
        public string Name { get; set; }
        public int Amount { get; set; }
    }

    public class RequiredItem
    {
        public string Name { get; set; }
        public int Amount { get; set; }
        public List<AvailableMaterial> Materials { get; set; }
        public AvailableMaterial SelectedMaterial { get; set; }
    }

    public enum BuildItemType
    {
        Workshop,
        Item,
        Terrain
    };

    public class BuildItem
    {
        public string Name { get; set; }
        public List<RequiredItem> RequiredItems { get; set; }
    }

    public class CommonModel
    {

        public double WindowWidth { get; set; }
        public double WindowHeight { get; set; }
    }

    public class GameModel : CommonModel
    {
        public string Day { get; set; }
        public string Year { get; set; }
        public string Time { get; set; }
        public string Sun { get; set; }

        public bool Paused { get; set; }
        public bool NormalSpeed { get; set; }
        public bool FastSpeed { get; set; }

        public System.Windows.Visibility ShowCommandButtons { get; set; }
        public List<CommandButton> CommandButtons { get; set; }

        public System.Windows.Visibility ShowCategoryButtons { get; set; }
        public List<BuildButton> BuildButtons { get; set; }
        public List<BuildItem> BuildItems { get; set; }

    }

    public class LoadGameModel
    {
        public class SaveItem
        {
            public string Name { get; set; }
            public string Version { get; set; }
            public string Data { get; set; }
        }

        public List<SaveItem> SavedKingdoms { get; set; }
        public SaveItem SelectedKingdom { get; set; }
        public List<SaveItem> SavedGames { get; set; }
        public SaveItem SelectedGame { get; set; }
    }

    public class ViewModel : CommonModel
    {
        public State State { get; set; }
        public string Platform { get; set; }

        public System.Windows.Visibility ShowMainMenu { get; set; }
        public System.Windows.Visibility ShowGameGUI { get; set; }
    }

    public partial class IngamePage : UserControl
    {
    }

    public partial class SettingsPage : UserControl
    {
    }

    public partial class LoadGamePage : UserControl
    {
    }

    public partial class Main : UserControl
    {
    }

    public class SpPriority
    {
        public string Name { get; set; }
    }

    public class MaterialItem
    {
        public string Name { get; set; }
        public bool Checked { get; set; }
    }

    public class ItemItem
    {
        public string Name { get; set; }
        public bool? Checked { get; set; }
        public bool Expanded { get; set; }
        public List<MaterialItem> Children { get; set; }
    }

    public class GroupItem
    {
        public string Name { get; set; }
        public bool? Checked { get; set; }
        public bool Expanded { get; set; }
        public List<ItemItem> Children { get; set; }
    }

    public class CategoryItem
    {
        public string Name { get; set; }
        public bool? Checked { get; set; }
        public bool Expanded { get; set; }
        public List<GroupItem> Children { get; set; }
    }

    public class StockpileModel
    {
        public string Name { get; set; }
        public bool Suspended { get; set; }
        public bool PullFromHere { get; set; }
        public bool PullFromOther { get; set; }
        public List<SpPriority> Priorities { get; set; }
        public SpPriority SelectedPrio { get; set; }
        public List<CategoryItem> Filters { get; set; }
    }

    public class NewGameModel
    {
    }

    public class TileInfoModel
    {
    }

    public class WorkshopModel
    {
    }

    public class SettingsModel
    {
    }

    public class PopulationModel
    {
    }

    public class AgricultureModel
    {
    }

    public class CreatureInfoModel
    {
    }

    public class DebugModel
    {
    }

    public partial class StockpileGui : UserControl
    {
    }

    public partial class PopulationWindow : UserControl
    {
    }

    public class MilitaryModel
    {
    }

    public class NeighborsModel
    {
    }
}
