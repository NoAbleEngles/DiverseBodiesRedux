# Fallout 4 GFx Menu

## Overview
This project implements a menu system for Fallout 4 using ActionScript 3 and the GFx CommonLibF4. The menu interacts with an external C++ application, providing a seamless user experience.

## Project Structure
```
fallout4-gfx-menu
├── src
│   ├── Main.as
│   ├── model
│   │   └── MenuModel.as
│   ├── view
│   │   ├── MenuView.as
│   │   └── components
│   │       └── ButtonComponent.as
│   ├── controller
│   │   └── MenuController.as
│   ├── utils
│   │   └── GFxBridge.as
│   └── types
│       └── MenuTypes.as
├── assets
│   └── fonts
│       └── FalloutFont.fdb
├── .vscode
│   └── launch.json
├── README.md
```

## Setup Instructions
1. **Clone the Repository**: 
   ```
   git clone <repository-url>
   cd fallout4-gfx-menu
   ```

2. **Install Dependencies**: Ensure you have the necessary tools and libraries for ActionScript 3 development and GFx CommonLibF4.

3. **Configure the Environment**: 
   - Open the project in your preferred IDE.
   - Adjust the `.vscode/launch.json` file if necessary to match your environment settings.

4. **Build the Project**: Follow the build instructions specific to your development environment to compile the ActionScript files.

## Usage
- Launch the application to see the menu in action.
- The menu allows users to interact with various options, which are managed through the `MenuController` and `MenuModel`.
- The `GFxBridge` utility facilitates communication with the C++ backend.

## Development
- **Main.as**: Entry point for initializing the menu system.
- **MenuModel.as**: Manages menu data and state.
- **MenuView.as**: Handles rendering and layout of the menu.
- **ButtonComponent.as**: Reusable button component for user interactions.
- **MenuController.as**: Manages user input and updates the menu state.
- **GFxBridge.as**: Utility functions for C++ interaction.
- **MenuTypes.as**: Defines types and constants used throughout the project.

## Contributing
Contributions are welcome! Please submit a pull request or open an issue for any enhancements or bug fixes.

## License
This project is licensed under the MIT License. See the LICENSE file for more details.