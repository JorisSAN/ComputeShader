#include "Scene_024_ComputeShaderPersonalized.h"
#include "../engine/Timer.h"
#include "../engine/MacroUtils.h"
#include "../engine/Log.h"

#include <cstdlib>
#include <ctime>
#include <GL/glew.h>


Scene_024_ComputeShaderPersonalized::Scene_024_ComputeShaderPersonalized() : totalTime(0), frameIndex(0) {

}

Scene_024_ComputeShaderPersonalized::~Scene_024_ComputeShaderPersonalized() {
    clean();
}

void Scene_024_ComputeShaderPersonalized::setGame(Game *_game) {
    game = _game;
}

void Scene_024_ComputeShaderPersonalized::clean() {

}

void Scene_024_ComputeShaderPersonalized::pause() {
}

void Scene_024_ComputeShaderPersonalized::resume() {
}

void Scene_024_ComputeShaderPersonalized::handleEvent(const InputState &inputState) {

}

void Scene_024_ComputeShaderPersonalized::load() {
    Assets::loadComputeShader(SHADER_COMP(SHADER_NAME), SHADER_ID(SHADER_NAME));
    Assets::loadShader(SHADER_VERT(SHADER_NAME), SHADER_FRAG(SHADER_NAME), "", "", "", SHADER_ID(SHADER_NAME));

    // This is position and normal data for a confetti
    static const Vector3 geometry[] =
    {
        // Positions
        Vector3( 1.0f,  1.0f,  1.0f),
        Vector3(-1.0f,  1.0f,  1.0f),
        Vector3( 1.0f, -1.0f,  1.0f),
        Vector3(-1.0f, -1.0f,  1.0f),
        Vector3( 1.0f,  1.0f, -1.0f),
        Vector3(-1.0f,  1.0f, -1.0f),
        Vector3( 1.0f, -1.0f, -1.0f),
        Vector3(-1.0f, -1.0f, -1.0f),

        // Normals
        Vector3(-2.0f, 0.0f,  0.0f),
        Vector3( 2.0f, 2.0f,  0.0f),
        Vector3(-2.0f, 0.0f,  0.0f),
        Vector3( 2.0f, 2.0f, -2.0f),
        Vector3(-2.0f, 0.0f, -2.0f),
        Vector3( 2.0f, 2.0f, -2.0f),
        Vector3(-2.0f, 0.0f, -2.0f),
        Vector3( 2.0f, 2.0f,  0.0f),
    };

    computeShader = Assets::getComputeShader(SHADER_ID(SHADER_NAME));
    renderShader = Assets::getShader(SHADER_ID(SHADER_NAME));

    glGenBuffers(2, flockBuffer);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, flockBuffer[0]);
    glBufferData(GL_SHADER_STORAGE_BUFFER, FLOCK_SIZE * sizeof(flock_member), NULL, GL_DYNAMIC_COPY);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, flockBuffer[1]);
    glBufferData(GL_SHADER_STORAGE_BUFFER, FLOCK_SIZE * sizeof(flock_member), NULL, GL_DYNAMIC_COPY);

    int i;

    glGenBuffers(1, &geometryBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, geometryBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(geometry), geometry, GL_STATIC_DRAW);

    glGenVertexArrays(2, flockRenderVao);

    for (i = 0; i < 2; i++)
    {
        glBindVertexArray(flockRenderVao[i]);
        glBindBuffer(GL_ARRAY_BUFFER, geometryBuffer);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (void *)(8 * sizeof(Vector3)));

        glBindBuffer(GL_ARRAY_BUFFER, flockBuffer[i]);
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(flock_member), NULL);
        glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(flock_member), (void *)sizeof(Vector4));
        glVertexAttribDivisor(2, 1);
        glVertexAttribDivisor(3, 1);

        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);
        glEnableVertexAttribArray(2);
        glEnableVertexAttribArray(3);
    }

    spawnParticles(2000);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
}

void Scene_024_ComputeShaderPersonalized::spawnParticles(int nbParticles) {
    if(nbParticles + particlesGlobal < FLOCK_SIZE) {
        particlesGlobal += nbParticles;
    }
    else {
        particlesGlobal = nbParticles;;
    }

    glBindBuffer(GL_ARRAY_BUFFER, flockBuffer[0]);
    flock_member * ptr = reinterpret_cast<flock_member *>(glMapBufferRange(GL_ARRAY_BUFFER, (particlesGlobal - nbParticles) * sizeof(flock_member), nbParticles * sizeof(flock_member), GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT));

    for (int i = 0; i < nbParticles; i++)
    {
        ptr[i].position = Vector3(0.0f, 0.0f, 0.0f);
        ptr[i].velocity = Vector3(randomFloat(), randomFloat(), randomFloat()) - Vector3(0.5f, 0.5f, 0.5f);
        ptr[i].timeSpawn = Vector3(totalTime, 0.0f, 0.0f);
    }

    glUnmapBuffer(GL_ARRAY_BUFFER);
}

void Scene_024_ComputeShaderPersonalized::update(float dt) {
    totalTime += dt;

    spawnParticles(particlePerFrame);

    computeShader.use();

    Vector3 start = Vector3(0.0f, 0.0f, 0.0f);
    Vector3 force = Vector3(0.0f, 0.85f, 0.0f);

    computeShader.setVector3f("start", start);
    computeShader.setVector3f("force", force);
    computeShader.setFloat("totalTime", totalTime);

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, flockBuffer[frameIndex]);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, flockBuffer[frameIndex ^ 1]);

    glDispatchCompute(NUM_WORKGROUPS, 1, 1);
}

void Scene_024_ComputeShaderPersonalized::draw()
{
    renderShader.use();

    Matrix4 mv_matrix = Matrix4::createLookAt(Vector3(0.0f, 0.0f, -100.0f),
                                            Vector3(0.0f, 0.0f, 0.0f),
                                            Vector3(0.0f, 1.0f, 0.0f));
    Matrix4 proj_matrix = Matrix4::createPerspectiveFOV(90.0f, game->windowWidth, game->windowHeight, 0.1f, 3000.0f);
    Matrix4 mvp = proj_matrix * mv_matrix;

    renderShader.setMatrix4("mvp", mvp);

    glBindVertexArray(flockRenderVao[frameIndex]);
    glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 8, FLOCK_SIZE);

    frameIndex ^= 1;
}
