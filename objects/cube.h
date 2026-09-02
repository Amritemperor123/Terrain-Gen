struct CubeParams
{
    int count = 30;                  // Number of cubes in the scene
    float width = 1.5f;              // Base width (X scale)
    float height = 1.5f;             // Base height (Y scale)
    float depth = 1.5f;              // Base depth (Z scale)
    float dimensionRandomness = 0.5f; // Randomness factor for dimensions [0.0 - 2.0]
    float scaleOfRandomness = 40.0f;  // Spatial distribution spread across terrain
    unsigned int seed = 1337;        // Random seed for reproducible placement
};
