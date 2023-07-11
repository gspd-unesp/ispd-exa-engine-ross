#ifndef ISPD_ROUTING_HPP
#define ISPD_ROUTING_HPP

#include <fstream>
#include <unordered_map>
#include <ispd/log/log.hpp>

namespace ispd {
namespace routing {

namespace {
static uint64_t szudzik(const uint32_t a, const uint32_t b) {
  const auto a64 = static_cast<uint64_t>(a);
  const auto b64 = static_cast<uint64_t>(b);
  return a64 >= b64 ? a64 * a64 + a64 + b64 : a64 + b64 * b64;
}
}; // namespace

struct route {
  /// \brief The path's length.
  unsigned length;

  /// \brief The path.
  tw_lpid *path;

  inline tw_lpid get(const std::size_t index) const {
    DEBUG({
      /// Checks if the index being accessed overflow the route's path.
      /// If so, the program is immediately aborted.
      if (index >= length)
        ispd_error("[Routing] Acessing an invalid route element. (Index: %zu, "
                   "Length: %u).",
                   index, length);
    });

    return path[index];
  }
};

struct routing_table {
  /// \brief The map containing the registered routes.
  std::unordered_map<uint64_t, const route *> routes;

  void load(const std::string &filepath) {
    std::ifstream file(filepath);

    /// Checks if the routing file could not be opened. If so, the program
    /// is immediately aborted.
    if (!file.is_open())
      ispd_error("[Routing] Routing file %s could not be opened.", filepath);

    /// Read each line from the file. Each line in the file contains a route
    /// indicating the source service, the destination service and the services'
    /// identifiers that composes the inner route's elements.
    for (std::string route_line; std::getline(file, route_line);) {
      /// The source identifier that will be read from the file.
      tw_lpid src;

      /// The destination identifier that will be read from the file.
      tw_lpid dest;

      /// Parse the route line obtaining the route structure containing
      /// the route's length and the route itself as well as the source
      /// and destination in the route.
      route *r = parse_route_line(route_line, src, dest);

      /// Add the route.
      add_route(src, dest, r);

      /// Print the debug.
      DEBUG({
        std::printf("Route [F: %lu, T: %lu, P: ", src, dest);
        for (int i = 0; i < r->length - 1; i++)
          std::printf("%lu -> ", r->path[i]);
        std::printf("%lu].\n", r->path[r->length - 1]);
      });
    }

    file.close();
  }

  const route *get_route(const tw_lpid src, const tw_lpid dest) const {
    return routes.at(szudzik(src, dest));
  }

private:
  /**
   * @brief A parsing stage is an enumeration that is used
   *        classify the current parsing stage of the routing
   *        table reader.
   */
  enum ParsingStage {
    /**
     * @brief This parsing stage indicates that the source service
     *        identifier is being parsed by the reader.
     */
    SOURCE_VERTEX,

    /**
     * @brief This parsing stage indicates that the destination service
     *        identifier is being parsed by the reader.
     */
    DESTINATION_VERTEX,

    /**
     * @brief This parsing stage indicates that the route service
     *        identifier is being parsed by the reader.
     */
    INNER_VERTEX
  };

  route *parse_route_line(const std::string &routeLine, tw_lpid &src,
                          tw_lpid &dest) {
    const std::size_t routeLineLength = routeLine.length();
    std::size_t whitespaceCount = 0;

    std::size_t pathLength = 0;
    std::size_t pathIndex = 0;
    tw_lpid *path = nullptr;

    // It counts the amount of whitespaces the route line contains.
    // With that information in hands, it is possible to conclude the
    // path length and allocate the exactly amount of path elements.
    for (std::size_t i = 0; i < routeLineLength; i++)
      if (routeLine[i] == ' ')
        whitespaceCount++;

    // It sets the path length and allocate the path elements.
    pathLength = whitespaceCount - 1;
    path = new tw_lpid[pathLength];

    std::size_t partStart = 0;
    std::size_t partLength = 0;
    ParsingStage stage = ParsingStage::SOURCE_VERTEX;

    for (std::size_t i = 0; i < routeLineLength; i++) {
      while (routeLine[i] != ' ' && i < routeLineLength) {
        partLength++;
        i++;
      }

      // It obtains a route part. That is represented by a sequence of
      // letters followed by a space, new line or end of file.
      const std::string routePart = routeLine.substr(partStart, partLength);

      switch (stage) {
      case ParsingStage::SOURCE_VERTEX:
        src = std::stoul(routePart);
        stage = ParsingStage::DESTINATION_VERTEX;
        break;
      case ParsingStage::DESTINATION_VERTEX:
        dest = std::stoul(routePart);
        stage = ParsingStage::INNER_VERTEX;
        break;
      case ParsingStage::INNER_VERTEX:
        path[pathIndex++] = std::stoul(routePart);
        break;
      default:
        ispd_error("[Routing] Unknown parsing stage.");
      }

      partStart = i + 1;
      partLength = 0;
    }

    route *r = new route;
    r->length = pathLength;
    r->path = path;

    return r;
  }

  void add_route(const tw_lpid src, const tw_lpid dest, const route *route) {
    routes.insert(std::make_pair(szudzik(src, dest), route));
  }
};

}; // namespace routing
}; // namespace ispd

extern ispd::routing::routing_table g_routing_table;

#endif // ISPD_ROUTING_HPP
