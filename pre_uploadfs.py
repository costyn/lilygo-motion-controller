Import("env")
import subprocess
import os

def build_webapp_before_uploadfs(source, target, env):
    """Build the webapp before uploading SPIFFS filesystem"""
    print("=" * 60)
    print("Building webapp before SPIFFS upload...")
    print("=" * 60)

    project_dir = env.get("PROJECT_DIR")
    webapp_dir = os.path.join(project_dir, "webapp")

    if not os.path.exists(webapp_dir):
        print(f"Warning: webapp directory not found at {webapp_dir}")
        return

    try:
        # Check if pnpm is available, fall back to npm
        try:
            subprocess.run(["pnpm", "--version"], capture_output=True, check=True)
            package_manager = "pnpm"
        except (subprocess.CalledProcessError, FileNotFoundError):
            package_manager = "npm"

        print(f"Using package manager: {package_manager}")

        # Run the build command
        result = subprocess.run(
            [package_manager, "run", "build"],
            cwd=webapp_dir,
            capture_output=False,
            text=True
        )

        if result.returncode != 0:
            print(f"Error: webapp build failed with exit code {result.returncode}")
            env.Exit(1)

        print("=" * 60)
        print("Webapp build completed successfully!")
        print("=" * 60)

    except Exception as e:
        print(f"Error building webapp: {e}")
        env.Exit(1)

# Add the pre-upload hook for uploadfs target
env.AddPreAction("uploadfs", build_webapp_before_uploadfs)
