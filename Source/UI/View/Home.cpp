#include "UI/View/Home.hpp"

#include "Base.hpp"
#include "Chicane/Game/Actor/Component/Mesh.hpp"

constexpr const SDL_DialogFileFilter m_modelFilters[] = {
    { "All (.obj)", "obj" },
    { "Wavefront (.obj)", "obj" },
    { nullptr, nullptr }
};

constexpr const SDL_DialogFileFilter m_textureFilters[] = {
    { "All (.jpg, .jpeg)", "jpg;jpeg" },
    { "JPEG (.jpg, .jpeg)", "jpg;jpeg" },
    { nullptr, nullptr }
};

namespace Editor
{
    HomeView::HomeView()
        : Chicane::Grid::View(
            "home",
            "Content/View/Home.grid"
        ),
        m_uiSelectedActor(std::make_any<std::string>("")),
        m_uiIsConsoleOpen(std::make_any<std::string>("false")),
        m_currentDirectory(""),
        m_selectedActor(nullptr)
    {
        m_uiActorTranslation.resize(3);
        m_uiActorTranslation[0] = std::make_any<std::string>("0.0");
        m_uiActorTranslation[1] = std::make_any<std::string>("0.0");
        m_uiActorTranslation[2] = std::make_any<std::string>("0.0");

        m_uiActorRotation.resize(3);
        m_uiActorRotation[0] = std::make_any<std::string>("0.0");
        m_uiActorRotation[1] = std::make_any<std::string>("0.0");
        m_uiActorRotation[2] = std::make_any<std::string>("0.0");

        m_uiActorScaling.resize(3);
        m_uiActorScaling[0] = std::make_any<std::string>("1.0");
        m_uiActorScaling[1] = std::make_any<std::string>("1.0");
        m_uiActorScaling[2] = std::make_any<std::string>("1.0");

        setupWatchers();

        setupUiTelemetry();
        setupUiActor();
        setupUiDirectory();
        setupUiAssetCreator();
        setupUiConsole();

        listDir(".");
    }

    std::uint32_t HomeView::getFPS(const Chicane::Grid::ComponentEvent& inEvent)
    {
        return Chicane::getTelemetry().frame.rate;
    }

    std::string HomeView::getFrametime(const Chicane::Grid::ComponentEvent& inEvent)
    {
        std::string frametime = std::to_string(Chicane::getTelemetry().frame.time);

        return std::string(frametime.begin(), frametime.end() - 5);
    }

    void HomeView::showLog(const Chicane::Log::Instance& inLog)
    {
        if (inLog.isEmpty())
        {
            return;
        }

        Chicane::Grid::Style style {};
        style.foregroundColor = inLog.color;

        Chicane::Grid::TextComponent::compileRaw(inLog.text, style);
    }

    void HomeView::showActor(Chicane::Actor* inActor)
    {
        if (!inActor)
        {
            return;
        }

        Chicane::Grid::ButtonComponent::Props props {};
        props.id                    = Chicane::Utils::sprint("%p", inActor);
        props.style.width           = Chicane::Grid::getSize("100%");
        props.style.height          = Chicane::Grid::getSize("4vh");
        props.style.backgroundColor = "#444444";
        props.onClick               = [&](const Chicane::Grid::ComponentEvent& inEvent)
        {
            m_selectedActor = inActor;

            const Chicane::Transform& transform = inActor->getTransform();

            m_uiActorTranslation[0] = std::make_any<std::string>(Chicane::Utils::sprint("%.2f", transform.translation.x));
            m_uiActorTranslation[1] = std::make_any<std::string>(Chicane::Utils::sprint("%.2f", transform.translation.y));
            m_uiActorTranslation[2] = std::make_any<std::string>(Chicane::Utils::sprint("%.2f", transform.translation.z));

            m_uiActorRotation[0] = std::make_any<std::string>(Chicane::Utils::sprint("%.2f", transform.rotation.x));
            m_uiActorRotation[1] = std::make_any<std::string>(Chicane::Utils::sprint("%.2f", transform.rotation.y));
            m_uiActorRotation[2] = std::make_any<std::string>(Chicane::Utils::sprint("%.2f", transform.rotation.z));

            m_uiActorScaling[0] = std::make_any<std::string>(Chicane::Utils::sprint("%.2f", transform.scale.x));
            m_uiActorScaling[1] = std::make_any<std::string>(Chicane::Utils::sprint("%.2f", transform.scale.y));
            m_uiActorScaling[2] = std::make_any<std::string>(Chicane::Utils::sprint("%.2f", transform.scale.z));

            m_uiSelectedActor = std::make_any<std::string>(props.id);

            return 0;
        };
        props._renderers.push_back(
            [&](const Chicane::Grid::ComponentEvent& inEvent)
            {
                Chicane::Grid::Style style {};
                style.horizontalAlignment = Chicane::Grid::Alignment::Center;
                style.verticalAlignment   = Chicane::Grid::Alignment::Center;

                Chicane::Grid::TextComponent::compileRaw(props.id, style);

                return 0;
            }
        );

        Chicane::Grid::ButtonComponent::compileRaw(props);
    }

    void HomeView::showDirectoryHistory(const std::string& inPath)
    {
        std::vector<std::string> splittedPath = Chicane::Utils::split(
            inPath,
            Chicane::FileSystem::SEPARATOR
        );

        const std::string& folderName = splittedPath.back();

        Chicane::Grid::ButtonComponent::Props props {};
        props.id = inPath;
        props.style.width = Chicane::Grid::getSize(
            "5vw",
            Chicane::Grid::Direction::Horizontal,
            Chicane::Grid::Position::Relative
        );
        props.style.height = Chicane::Grid::getSize(
            Chicane::Grid::AUTO_SIZE_UNIT,
            Chicane::Grid::Direction::Vertical,
            Chicane::Grid::Position::Relative
        );
        props.style.backgroundColor = "#444444";
        props.onClick = [&](const Chicane::Grid::ComponentEvent& inEvent)
        {
            listDir(inPath);

            return 0;
        };
        props._renderers.push_back(
            [&](const Chicane::Grid::ComponentEvent& inEvent)
            {
                Chicane::Grid::Style style {};
                style.horizontalAlignment = Chicane::Grid::Alignment::Center;
                style.verticalAlignment   = Chicane::Grid::Alignment::Center;

                Chicane::Grid::TextComponent::compileRaw(folderName, style);

                return 0;
            }
        );

        Chicane::Grid::ButtonComponent::compileRaw(props);
    }

    void HomeView::showDirectory(const Chicane::FileSystem::ListItem& inItem)
    {
        Chicane::Grid::ButtonComponent::Props props {};
        props.id = inItem.path;
        props.style.width = Chicane::Grid::getSize(
            Chicane::Grid::AUTO_SIZE_UNIT,
            Chicane::Grid::Direction::Horizontal,
            Chicane::Grid::Position::Relative
        );
        props.style.height = Chicane::Grid::getSize(
            Chicane::Grid::AUTO_SIZE_UNIT,
            Chicane::Grid::Direction::Vertical,
            Chicane::Grid::Position::Relative
        );
        props.style.backgroundColor = "#444444";

        if (inItem.type == Chicane::FileSystem::ListType::Folder)
        {
            props.onClick = [&](const Chicane::Grid::ComponentEvent& inEvent)
            {
                listDir(inItem.path);

                return 0;
            };
            props._renderers.push_back(
                [&](const Chicane::Grid::ComponentEvent& inEvent)
                {
                    Chicane::Grid::Style style {};
                    style.horizontalAlignment = Chicane::Grid::Alignment::Center;
                    style.verticalAlignment   = Chicane::Grid::Alignment::Center;

                    std::string title = inItem.name + " - " + std::to_string(inItem.childCount) + " Items";

                    Chicane::Grid::TextComponent::compileRaw(title, style);

                    return 0;
                }
            );
        }

        if (inItem.type == Chicane::FileSystem::ListType::File)
        {
            props.onClick = [&](const Chicane::Grid::ComponentEvent& inEvent)
            {
                Chicane::addActor(new MeshActor(inItem.path));

                return 0;
            };
            props._renderers.push_back(
                [&](const Chicane::Grid::ComponentEvent& inEvent)
                {
                    Chicane::Grid::Style style {};
                    style.horizontalAlignment = Chicane::Grid::Alignment::Center;
                    style.verticalAlignment   = Chicane::Grid::Alignment::Center;

                    Chicane::Grid::TextComponent::compileRaw(inItem.name, style);

                    return 0;
                }
            );
        }

        Chicane::Grid::ButtonComponent::compileRaw(props);
    }

    void HomeView::setupWatchers()
    {
        Chicane::Log::watchLogs(
            [this](const Chicane::Log::List& inLogs)
            {
                std::vector<std::any> logs;

                for (const Chicane::Log::Instance& log : inLogs)
                {
                    logs.push_back(std::make_any<Chicane::Log::Instance>(log));
                }

                m_uiConsoleLogs = std::make_any<std::vector<std::any>>(logs);
            }
        );
        Chicane::watchActiveLevel(
            [this](Chicane::Level* inLevel)
            {
                if (!inLevel)
                {
                    updateOutliner();
                }

                inLevel->watchActors(
                    [this](Chicane::Actor* inActor)
                    {
                        updateOutliner();
                    }
                );
            }
        );
    }

    void HomeView::setupUiTelemetry()
    {
        // Functions
        addFunction(
            "getFPS",
            std::bind(&HomeView::getFPS, this, std::placeholders::_1)
        );
        addFunction(
            "getFrametime",
            std::bind(&HomeView::getFrametime, this, std::placeholders::_1)
        );
    }

    void HomeView::setupUiActor()
    {
        // Variables
        addVariable(
            "actors",
            &m_uiActors
        );
        addVariable(
            "selectedActor",
            &m_uiSelectedActor
        );
        addVariable(
            "actorTranslationX",
            &m_uiActorTranslation[0]
        );
        addVariable(
            "actorTranslationY",
            &m_uiActorTranslation[1]
        );
        addVariable(
            "actorTranslationZ",
            &m_uiActorTranslation[2]
        );
        addVariable(
            "actorRotationX",
            &m_uiActorRotation[0]
        );
        addVariable(
            "actorRotationY",
            &m_uiActorRotation[1]
        );
        addVariable(
            "actorRotationZ",
            &m_uiActorRotation[2]
        );
        addVariable(
            "actorScalingX",
            &m_uiActorScaling[0]
        );
        addVariable(
            "actorScalingY",
            &m_uiActorScaling[1]
        );
        addVariable(
            "actorScalingZ",
            &m_uiActorScaling[2]
        );

        // Functions
        addFunction(
            "showActor",
            [this](const Chicane::Grid::ComponentEvent& inEvent)
            {
                showActor(std::any_cast<Chicane::Actor*>(inEvent.values[0]));

                return 0;
            }
        );
        addFunction(
            "onTranslationChange",
            [this](const Chicane::Grid::ComponentEvent& inEvent)
            {
                updateTranslation();

                return 0;
            }
        );
        addFunction(
            "onRotationChange",
            [this](const Chicane::Grid::ComponentEvent& inEvent)
            {
                updateRotation();

                return 0;
            }
        );
        addFunction(
            "onScalingChange",
            [this](const Chicane::Grid::ComponentEvent& inEvent)
            {
                updateScaling();

                return 0;
            }
        );
    }

    void HomeView::setupUiDirectory()
    {
        // Variables
        addVariable(
            "directoryHistory",
            &m_uiDirectoryHistory
        );
        addVariable(
            "directoryInfo",
            &m_uiDirectoryInfo
        );

        // Functions
        addFunction(
            "showDirectoryHistory",
            [this](const Chicane::Grid::ComponentEvent& inEvent)
            {
                showDirectoryHistory(std::any_cast<std::string>(inEvent.values[0]));

                return 0;
            }
        );
        addFunction(
            "showDirectory",
            [this](const Chicane::Grid::ComponentEvent& inEvent)
            {
                showDirectory(std::any_cast<Chicane::FileSystem::ListItem>(inEvent.values[0]));

                return 0;
            }
        );
    }

    void HomeView::setupUiAssetCreator()
    {
        // Functions
        addFunction(
            "showModelCreator",
            [this](const Chicane::Grid::ComponentEvent& inEvent)
            {
                Chicane::FileSystem::FileDialog props {};
                props.title         = "Select 3D Model";
                props.filters       = m_modelFilters;
                props.filterCount   = 2;
                props.canSelectMany = false;

                Chicane::FileSystem::openFileDialog(
                    props,
                    [](void* inData, const char* const* inFiles, int inFilter)
                    {
                        const auto& result = Chicane::FileSystem::DialogResult::fromRaw(inFiles);

                        if (result.empty())
                        {
                            return;
                        }

                        const auto& path = result.at(0).path;

                        Chicane::Box::WriteEntry entry {};
                        entry.type         = Chicane::Box::EntryType::Model;
                        entry.dataFilePath = path.string();

                        Chicane::Box::WriteInfo writeInfo {};
                        writeInfo.type         = Chicane::Box::Type::Model;
                        writeInfo.name         = path.filename().string();
                        writeInfo.outputFolder = "Content/Models/";
                        writeInfo.entries.push_back(entry);

                        Chicane::Box::write(writeInfo);
                    }
                );

                return 0;
            }
        );
        addFunction(
            "showTextureCreator",
            [this](const Chicane::Grid::ComponentEvent& inEvent)
            {
                Chicane::FileSystem::FileDialog props {};
                props.title         = "Select a Texture Image";
                props.filters       = m_textureFilters;
                props.filterCount   = 2;
                props.canSelectMany = false;

                Chicane::FileSystem::openFileDialog(
                    props,
                    [](void* inData, const char* const* inFiles, int inFilter)
                    {
                        const auto& result = Chicane::FileSystem::DialogResult::fromRaw(inFiles);

                        if (result.empty())
                        {
                            return;
                        }

                        const auto& path = result.at(0).path;

                        Chicane::Box::WriteEntry entry {};
                        entry.type         = Chicane::Box::EntryType::Texture;
                        entry.dataFilePath = result.at(0).path.string();

                        Chicane::Box::WriteInfo writeInfo {};
                        writeInfo.type         = Chicane::Box::Type::Texture;
                        writeInfo.name         = path.filename().string();
                        writeInfo.outputFolder = "Content/Textures/";
                        writeInfo.entries.push_back(entry);

                        Chicane::Box::write(writeInfo);
                    }
                );

                return 0;
            }
        );
    }

    void HomeView::setupUiConsole()
    {
        // Variables
        addVariable(
            "consoleLogs",
            &m_uiConsoleLogs
        );
        addVariable(
            "isConsoleOpen",
            &m_uiIsConsoleOpen
        );

        // Functions
        addFunction(
            "showConsole",
            [this](const Chicane::Grid::ComponentEvent& inEvent)
            {
                m_uiIsConsoleOpen = std::make_any<std::string>("true");

                return 0;
            }
        );
        addFunction(
            "hideConsole",
            [this](const Chicane::Grid::ComponentEvent& inEvent)
            {
                m_uiIsConsoleOpen = std::make_any<std::string>("false");

                return 0;
            }
        );
        addFunction(
            "showLog",
            [this](const Chicane::Grid::ComponentEvent& inEvent)
            {
                showLog(std::any_cast<Chicane::Log::Instance>(inEvent.values[0]));

                return 0;
            }
        );
    }

    void HomeView::updateTranslation()
    {
        if (!m_selectedActor)
        {
            return;
        }

        std::string x = std::any_cast<std::string>(m_uiActorTranslation[0]);
        std::string y = std::any_cast<std::string>(m_uiActorTranslation[1]);
        std::string z = std::any_cast<std::string>(m_uiActorTranslation[2]);

        m_selectedActor->setAbsoluteTranslation(
            Chicane::Vec<3, float>(
                x.empty() ? 0.0f : std::stof(x),
                y.empty() ? 0.0f : std::stof(y),
                z.empty() ? 0.0f : std::stof(z)
            )
        );
    }

    void HomeView::updateRotation()
    {
        if (!m_selectedActor)
        {
            return;
        }

        std::string x = std::any_cast<std::string>(m_uiActorRotation[0]);
        std::string y = std::any_cast<std::string>(m_uiActorRotation[1]);
        std::string z = std::any_cast<std::string>(m_uiActorRotation[2]);

        m_selectedActor->setAbsoluteRotation(
            Chicane::Vec<3, float>(
                x.empty() ? 0.0f : std::stof(x),
                y.empty() ? 0.0f : std::stof(y),
                z.empty() ? 0.0f : std::stof(z)
            )
        );
    }

    void HomeView::updateScaling()
    {
        if (!m_selectedActor)
        {
            return;
        }

        std::string x = std::any_cast<std::string>(m_uiActorScaling[0]);
        std::string y = std::any_cast<std::string>(m_uiActorScaling[1]);
        std::string z = std::any_cast<std::string>(m_uiActorScaling[2]);

        m_selectedActor->setAbsoluteScale(
            Chicane::Vec<3, float>(
                x.empty() ? 1.0f : std::stof(x),
                y.empty() ? 1.0f : std::stof(y),
                z.empty() ? 1.0f : std::stof(z)
            )
        );
    }

    void HomeView::updateOutliner()
    {
        std::unordered_map<std::string, std::uint32_t> ids {};

        std::vector<std::any> actors {};

        for (Chicane::Actor* actor : Chicane::getActors())
        {
            actors.push_back(std::make_any<Chicane::Actor*>(actor));
        }

        m_uiActors = std::make_any<std::vector<std::any>>(actors);
    }

    void HomeView::updateDirHistory()
    {
        std::vector<std::any> items = {};

        std::vector<std::string> splittedPath = Chicane::Utils::split(
            m_currentDirectory,
            Chicane::FileSystem::SEPARATOR
        );

        for (std::uint32_t i = 0; i < splittedPath.size(); i++)
        {
            std::string path = "";
            
            for (std::uint32_t j = 0; j <= i; j++)
            {
                path += splittedPath.at(j);
                path += Chicane::FileSystem::SEPARATOR;
            }

            items.push_back(
                std::make_any<std::string>(path)
            );
        }

        m_uiDirectoryHistory = std::make_any<std::vector<std::any>>(items);
    }

    void HomeView::listDir(const std::string& inPath)
    {
        std::vector<std::any> items = {};

        for (const auto& item : Chicane::FileSystem::ls(inPath))
        {
            if (item.type == Chicane::FileSystem::ListType::Folder)
            {
                items.push_back(
                    std::make_any<Chicane::FileSystem::ListItem>(item)
                );

                continue;
            }

            std::string extension = item.extension;

            if (!Chicane::Utils::areEquals(extension, ".box") && !Chicane::Utils::areEquals(extension, ".grid"))
            {
                continue;
            }

            items.push_back(
                std::make_any<Chicane::FileSystem::ListItem>(item)
            );
        }

        m_uiDirectoryInfo = std::make_any<std::vector<std::any>>(items);

        m_currentDirectory = inPath;

        updateDirHistory();
    }
}