#ifndef THEME_ROLES_H
#define THEME_ROLES_H

enum class ColorRole {
    // Background roles
    Background,
    BackgroundAlternate,
    BackgroundDisabled,
    BackgroundHighlight,
    
    // Foreground roles
    Foreground,
    ForegroundDisabled,
    ForegroundInactive,
    ForegroundLink,
    ForegroundVisited,
    ForegroundHighlight,
    
    // Control roles
    Button,
    ButtonText,
    ButtonHover,
    ButtonPressed,
    ButtonDisabled,
    ButtonBorder,
    
    // Input roles
    Input,
    InputText,
    InputDisabled,
    InputBorder,
    InputPlaceholder,
    InputSelection,
    InputSelectionText,
    
    // Window decoration roles
    TitleBar,
    TitleBarText,
    TitleBarInactive,
    TitleBarInactiveText,
    Border,
    
    // Status roles
    Success,
    Warning,
    Error,
    Info,
    
    // Accent roles
    Primary,
    Secondary,
    Tertiary,
    
    // Focus roles
    FocusOutline,
    FocusBackground,
    
    // Divider roles
    Divider,
    DividerAlternate,
    
    // Hover/active roles
    HoverOverlay,
    ActiveOverlay,
    
    // Custom roles
    Custom1,
    Custom2,
    Custom3
};

#endif // THEME_ROLES_H