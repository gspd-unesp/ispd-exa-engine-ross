#include <ross.h>
#include <ispd/routing/routing.hpp>

namespace ispd::routing {

namespace {
/// \enum ParsingStage
///
/// \brief An enumeration representing different stages of parsing in a
///        reader.
///
/// The `ParsingStage` enum defines three distinct stages of parsing that can
/// occur while reading the routing line. Each stage corresponds to the
/// parsing of a specific service identifier, namely, the source service,
/// destination service, and route service identifiers.
///
enum class ParsingStage {
  /// \brief This parsing stage indicates that the source service identifier
  ///        is being parsed by the reader.
  ///
  /// The reader is currently processing the source service identifier, which
  /// identifies the source vertex of the route.
  ///
  SOURCE_VERTEX,

  /// \brief This parsing stage indicates that the destination service
  ///        identifier is being parsed by the reader.
  ///
  /// The reader is currently processing the destination service identifier,
  /// which identifies the destination vertex of the route.
  ///
  DESTINATION_VERTEX,

  /// \brief This parsing stage indicates that the route service identifier is
  ///        being parsed by the reader.
  ///
  /// The reader is currently processing the route service identifier, which
  /// represents an intermediate vertex along a path or route.
  ///
  INNER_VERTEX
};
}; // namespace

auto RoutingTable::addRoute(const tw_lpid src, const tw_lpid dest,
                            const Route *route) -> void {
  /// It counts how many routes have been registered starting from this source
  /// vertex. This is done for the purpose of provide an early sanity check
  /// about if the routes have been registered correctly with relation to the
  /// model built.
  m_RoutesCounting[src]++;

  /// Insert the route.
  m_Routes[szudzik(src, dest)].push_back(route);
}

auto RoutingTable::parseRouteLine(const std::string &routeLine,
                                  const std::size_t lineNumber, tw_lpid &src,
                                  tw_lpid &dest) -> Route * {
  const std::size_t routeLineLength = routeLine.length();
  std::size_t whitespaceCount = 0;
  std::size_t pathLength = 0;
  std::size_t pathIndex = 0;

  // It counts the amount of whitespaces the route line contains.
  // With that information in hands, it is possible to conclude the
  // path length and allocate the exactly amount of path elements.
  for (std::size_t i = 0; i < routeLineLength; i++)
    if (routeLine[i] == ' ')
      whitespaceCount++;

  // It sets the path length and allocate the path elements.
  pathLength = whitespaceCount - 1;

  std::unique_ptr<tw_lpid *> path =
      std::make_unique<tw_lpid *>(new tw_lpid[pathLength]);

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

#define TRY_CATCH_PARSE(LEFT, TYPENAME)                                        \
  try {                                                                        \
    LEFT = std::stoul(routePart);                                              \
  } catch (const std::invalid_argument &e) {                                   \
    ispd_error(TYPENAME " vertex is not a number (Line Number: %lu).",         \
               lineNumber);                                                    \
  } catch (const std::out_of_range &e) {                                       \
    ispd_error(TYPENAME "vertex is out of range (Line Number: %lu).",          \
               lineNumber);                                                    \
  }

    switch (stage) {
    case ParsingStage::SOURCE_VERTEX:
      TRY_CATCH_PARSE(src, "Source")
      stage = ParsingStage::DESTINATION_VERTEX;
      break;
    case ParsingStage::DESTINATION_VERTEX:
      TRY_CATCH_PARSE(dest, "Destination")
      stage = ParsingStage::INNER_VERTEX;
      break;
    case ParsingStage::INNER_VERTEX:
      TRY_CATCH_PARSE((*path)[pathIndex++], "Inner")
      break;
    default:
      ispd_error("Unknown parsing stage while parsing a route line.");
    }

#undef TRY_CATCH_PARSE

    partStart = i + 1;
    partLength = 0;
  }

  /// Creates the route object contanining the path that has been
  /// read from the specified routing line and the route's length.
  return new Route(std::move(path), pathLength);
}

auto RoutingTable::load(const std::string &filepath) -> void {
  std::ifstream file(filepath);
  std::size_t lineNumber = 0;

  /// Check if the routing file could not be opened. If so, an error
  /// indicating the case is sent and the program is immediately aborted.
  if (!file.is_open()) [[unlikely]]
    ispd_error("Routing file %s could not be opened.", filepath.c_str());

  /// Read the each line from the routing file. Each line in the file
  /// represents a route indicating the source service, the destination
  /// service and the services' identifiers that composes the inner route's
  /// elements.
  ///
  /// In that case, each line from the routing file has the following format:
  ///
  ///                    <SRC-ID> <DEST-ID> [<ID>]
  ///
  /// in which, [<ID>] indicates one or more identifiers.
  for (std::string routeLine; std::getline(file, routeLine);) {
    lineNumber++;

    /// The source and destination identifier that will be identified from
    /// the route line.
    tw_lpid src, dest;

    /// Parse the route line obtaining the route structure containing the
    /// route's length and the route's intermediate services identifiers.
    Route *route = parseRouteLine(routeLine, lineNumber, src, dest);

    /// Add the route.
    addRoute(src, dest, route);

    /// Print the loaded route.
    DEBUG({
      std::printf("Route [F: %lu, T: %lu, P: ", src, dest);
      for (int i = 0; i < route->getLength() - 1; i++)
        std::printf("%lu -> ", route->get(i));
      std::printf("%lu].\n", route->get(route->getLength() - 1));
    });
  }
}

auto RoutingTable::getRoute(const tw_lpid src, const tw_lpid dest) const
    -> const Route * {
  return m_Routes.at(szudzik(src, dest))[0];
}

auto RoutingTable::countRoutes(const tw_lpid src) const -> const std::uint32_t {
  const auto it = m_RoutesCounting.find(src);
  if (it == m_RoutesCounting.end())
    ispd_error("There is no routing with source at LP with GID %lu.", src);

  return m_RoutesCounting.at(src);
}

}; // namespace ispd::routing

namespace ispd::routing_table {
/// \brief The global routing table.
ispd::routing::RoutingTable *g_RoutingTable = new ispd::routing::RoutingTable();

auto load(const std::string &filepath) -> void {
  /// Forward the route tabl load to the global routing table.
  g_RoutingTable->load(filepath);
}

auto getRoute(const tw_lpid src, const tw_lpid dest)
    -> const ispd::routing::Route * {
  /// Forward the route query to the global routing table.
  return g_RoutingTable->getRoute(src, dest);
}

auto countRoutes(const tw_lpid src) -> const std::uint32_t {
  /// Forward the route couting to the global routing table.
  return g_RoutingTable->countRoutes(src);
}

}; // namespace ispd::routing_table
