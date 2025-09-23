import React, { createContext, useContext, useEffect, useState } from "react"
import { storage, THEME_KEY, applyTheme, type Theme } from "@/lib/utils"

type ThemeProviderProps = {
    children: React.ReactNode
    defaultTheme?: Theme
}

const ThemeProviderContext = createContext<{
    theme: Theme
    setTheme: (theme: Theme) => void
}>({
    theme: "system",
    setTheme: () => null,
})

export function ThemeProvider({
    children,
    defaultTheme = "system",
}: ThemeProviderProps) {
    const [theme, setTheme] = useState<Theme>(() => {
        // Load theme from localStorage on initialization
        return storage.get(THEME_KEY, defaultTheme)
    })

    useEffect(() => {
        // Apply theme to DOM
        applyTheme(theme)
    }, [theme])

    const updateTheme = (newTheme: Theme) => {
        setTheme(newTheme)
        storage.set(THEME_KEY, newTheme)
    }

    useEffect(() => {
        // Listen for system theme changes when using 'system' theme
        if (theme === 'system') {
            const mediaQuery = window.matchMedia('(prefers-color-scheme: dark)')
            const handleChange = () => applyTheme(theme)

            mediaQuery.addEventListener('change', handleChange)
            return () => mediaQuery.removeEventListener('change', handleChange)
        }
    }, [theme])

    return (
        <ThemeProviderContext.Provider value={{ theme, setTheme: updateTheme }}>
            {children}
        </ThemeProviderContext.Provider>
    )
}

export const useTheme = () => {
    const context = useContext(ThemeProviderContext)
    if (!context) throw new Error("useTheme must be used within a ThemeProvider")
    return context
}