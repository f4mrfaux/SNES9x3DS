#!/bin/bash
#
# Clone Required External Repositories
# SNES 3D Layer Separation Project
#

set -e  # Exit on error

echo "========================================="
echo "SNES 3D Layer Separation - Repo Cloner"
echo "========================================="
echo ""

# Create repos directory
echo "Creating repos directory..."
mkdir -p repos
cd repos

# Clone SNES9x forks
echo ""
echo "[1/4] Cloning matbo87/snes9x_3ds (main fork)..."
if [ -d "matbo87-snes9x_3ds" ]; then
    echo "  ⚠️  Directory exists, skipping..."
else
    git clone https://github.com/matbo87/snes9x_3ds.git matbo87-snes9x_3ds
    echo "  ✅ Done"
fi

echo ""
echo "[2/4] Cloning bubble2k16/snes9x_3ds (original)..."
if [ -d "bubble2k16-snes9x_3ds" ]; then
    echo "  ⚠️  Directory exists, skipping..."
else
    git clone https://github.com/bubble2k16/snes9x_3ds.git bubble2k16-snes9x_3ds
    echo "  ✅ Done"
fi

echo ""
echo "[3/4] Cloning devkitPro/3ds-examples..."
if [ -d "devkitpro-3ds-examples" ]; then
    echo "  ⚠️  Directory exists, skipping..."
else
    git clone https://github.com/devkitPro/3ds-examples.git devkitpro-3ds-examples
    echo "  ✅ Done"
fi

echo ""
echo "[4/4] Cloning devkitPro/citro3d (library source)..."
if [ -d "devkitpro-citro3d" ]; then
    echo "  ⚠️  Directory exists, skipping..."
else
    git clone https://github.com/devkitPro/citro3d.git devkitpro-citro3d
    echo "  ✅ Done"
fi

cd ..

echo ""
echo "========================================="
echo "✅ All repositories cloned successfully!"
echo "========================================="
echo ""
echo "Next steps:"
echo "  1. Build SNES9x:  cd repos/matbo87-snes9x_3ds && make -j4"
echo "  2. Read docs:     docs/IMPLEMENTATION_ROADMAP.md"
echo "  3. Start coding!  🎮"
echo ""
