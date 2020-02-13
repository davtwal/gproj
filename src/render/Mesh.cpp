// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// * gproj : Mesh.cpp
// * Copyright (C) DigiPen Institute of Technology 2019
// * 
// * Created     : 2019y 09m 26d
// * Last Altered: 2019y 12m 26d
// * 
// * Author      : David Walker
// * E-mail      : d.walker\@digipen.edu
// * 
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// * Description :

#include "render/Mesh.h"
#include "render/CommandBuffer.h"
#include "render/Queue.h"


namespace dw {
  Mesh::Mesh(std::vector<Vertex> vertices, std::vector<uint32_t> indices)
    : m_vertices(std::move(vertices)),
      m_indices(std::move(indices)) {
  }

  Mesh::Mesh(Mesh&& o) noexcept
    : m_vertices(std::move(o.m_vertices)),
      m_indices(std::move(o.m_indices)),
      m_vertexBuff(std::move(o.m_vertexBuff)),
      m_indexBuff(std::move(o.m_indexBuff)),
      m_material(std::move(o.m_material)) {
    o.m_vertexBuff = nullptr;
    o.m_indexBuff  = nullptr;
    o.m_material   = nullptr;
  }

  Mesh::~Mesh() {
    if (m_vertexBuff)
      m_vertexBuff.reset();

    if (m_indexBuff)
      m_indexBuff.reset();

    if (m_material)
      m_material.reset();
  }

  Mesh& Mesh::setMaterial(util::ptr<Material> mtl) {
    m_material = std::move(mtl);
    return *this;
  }

  std::string const& Mesh::getName() const {
    return m_name;
  }


  util::ptr<Material> Mesh::getMaterial() const {
    return m_material;
  }

  Buffer& Mesh::getIndexBuffer() const {
    return *m_indexBuff;
  }

  Buffer& Mesh::getVertexBuffer() const {
    return *m_vertexBuff;
  }

  Mesh& Mesh::operator=(Mesh&& o) noexcept {
    m_vertices   = std::move(o.m_vertices);
    m_indices    = std::move(o.m_indices);
    m_vertexBuff = std::move(o.m_vertexBuff);
    m_indexBuff  = std::move(o.m_indexBuff);
    m_material   = std::move(m_material);
    return *this;
  }

  size_t Mesh::getSizeOfIndices() const {
    return m_indices.size() * sizeof(uint32_t);
  }

  size_t Mesh::getSizeOfVertices() const {
    return m_vertices.size() * sizeof(Vertex);
  }

  size_t Mesh::getIndexSize() const {
    return sizeof(uint32_t);
  }

  size_t Mesh::getVertexSize() const {
    return sizeof(Vertex);
  }

  size_t Mesh::getNumIndices() const {
    return m_indices.size();
  }

  size_t Mesh::getNumVertices() const {
    return m_vertices.size();
  }

  void Mesh::createBuffers(LogicalDevice& device) {
    auto vertSize  = getSizeOfVertices();
    auto indexSize = getSizeOfIndices();

    m_vertexBuff = std::make_unique<Buffer>(Buffer::CreateVertex(device, vertSize));
    m_indexBuff  = std::make_unique<Buffer>(Buffer::CreateIndex(device, indexSize));
  }

  void Mesh::uploadStaging(StagingBuffs& buffs) {
    void* data = buffs.first.map();
    memcpy(data, m_vertices.data(), getSizeOfVertices());
    buffs.first.unMap();

    data = buffs.second.map();
    memcpy(data, m_indices.data(), getSizeOfIndices());
    buffs.second.unMap();
  }

  Mesh::StagingBuffs Mesh::createStaging(LogicalDevice& device) const {
    return std::make_pair(
                          Buffer::CreateStaging(device, getSizeOfVertices()),
                          Buffer::CreateStaging(device, getSizeOfIndices())
                         );
  }

  void Mesh::uploadCmds(CommandBuffer& cmdBuff, StagingBuffs& buffs) const {
    VkBufferCopy vertCopy  = {0, 0, getSizeOfVertices()};
    VkBufferCopy indexCopy = {0, 0, getSizeOfIndices()};

    assert(m_vertexBuff);
    assert(m_indexBuff);

    vkCmdCopyBuffer(cmdBuff, buffs.first, *m_vertexBuff, 1, &vertCopy);
    vkCmdCopyBuffer(cmdBuff, buffs.second, *m_indexBuff, 1, &indexCopy);
  }

  Mesh::StagingBuffs Mesh::upload(CommandBuffer& cmdBuff) {
    LogicalDevice& device    = cmdBuff.getOwningPool().getOwningDevice();
    auto           vertSize  = getSizeOfVertices();
    auto           indexSize = getSizeOfIndices();

    createBuffers(device);
    auto staging = createStaging(device);
    uploadStaging(staging);
    uploadCmds(cmdBuff, staging);

    return staging;
  }

  void Mesh::upload(CommandBuffer& buffer, Queue& queue) {
    buffer.start(true);
    auto staging = upload(buffer);
    buffer.end();

    queue.submitOne(buffer);
    queue.waitSubmit();
  }

  void Mesh::clearCache() {
    m_vertices.clear();
    m_indices.clear();
  }

  Mesh& Mesh::calculateTangents() {
    for (size_t i = 0; i < m_indices.size(); i += 3) {
      auto& v0 = m_vertices[m_indices[i]];
      auto& v1 = m_vertices[m_indices[i + 1]];
      auto& v2 = m_vertices[m_indices[i + 2]];

      glm::vec3 deltaP0 = v1.pos - v0.pos;
      glm::vec3 deltaP1 = v2.pos - v0.pos;

      glm::vec2 deltaUV0 = v1.texCoord - v0.texCoord;
      glm::vec2 deltaUV1 = v2.texCoord - v0.texCoord;

      float denom = 1.f / (deltaUV0.x * deltaUV1.y - deltaUV0.y * deltaUV1.x);

      glm::vec3 tan  = (deltaP0 * deltaUV1.y - deltaP1 * deltaUV0.y) * denom;
      glm::vec3 btan = (deltaP1 * deltaUV0.x - deltaP0 * deltaUV1.x) * denom;

      v0.tangent += tan;
      v1.tangent += tan;
      v2.tangent += tan;

      v0.bitangent += btan;
      v1.bitangent += btan;
      v2.bitangent += btan;
    }

    for (auto& vert : m_vertices) {
      vert.tangent   = normalize(vert.tangent);
      vert.bitangent = normalize(vert.bitangent);
    }
    return *this;
  }


  bool Mesh::isDrawable() const {
    return m_vertexBuff != nullptr;
  }

  bool Mesh::operator==(Mesh const& o) const {
    return m_vertexBuff == o.m_vertexBuff;
  }
}
