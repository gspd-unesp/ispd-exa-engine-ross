#ifndef ISPD_ROUTING_HPP
#define ISPD_ROUTING_HPP

#include <ross.h>
#include <memory>
#include <cstdint>
#include <fstream>
#include <numeric>
#include <type_traits>
#include <unordered_map>
#include <ispd/log/log.hpp>

namespace ispd::routing {

namespace {

/// \brief Checks if %T type can be used in Szudzik's pairing function.
///
/// This value is %true, if and only if, the type can be used in the
/// Szudzik's pairing function. The restrctions are that the type must
/// be an unsigned integer type and must not be unsigned char.
///
/// \tparam T Type to be checked.
template <typename T>
inline constexpr bool is_szudzik_type_v =
    std::is_integral_v<T> && std::is_unsigned_v<T> &&
    !std::is_same_v<T, unsigned char>;

/// \brief Szudzik's Pairing Function
///
/// This static function implements Szudzik's Pairing Function, which combines
/// two non-negative 32-bit integers 'a' and 'b' into a unique 64-bit unsigned
/// integer result. The returned integer is a combination of 'a' and 'b' that
/// ensures uniqueness for each pair (a, b).
///
/// Szudzik's pairing function provides a way to map a pair of integers onto a
/// single value, useful for hashing or indexing two-dimensional data
/// structures. It is a method to transform a pair of non-negative integers into
/// a single integer, allowing two-dimensional data to be represented as a
/// one-dimensional index.
///
/// The pairing function is defined as follows:
///
///     result = a >= b ? a * a + a + b : a + b * b
///
/// \param a The first non-negative 32-bit integer.
/// \param b The second non-negative 32-bit integer.
/// \return A 64-bit integer representing the unique pairing of 'a' and 'b'.
///
/// \note Szudzik's pairing function is invertible, meaning that it can be
///       reversed to extract 'a' and 'b' from the result.
//
///       However, it may lead to information loss if the original 'a' and 'b'
///       values exceed the 32-bit range. For larger integers, consider using
///       more advanced pairing functions or hash functions. The function
///       assumes that 'a' and 'b' are non-negative 32-bit integers. It may not
///       be suitable for negative integers or other data types without
///       modification. For mathematical details, refer to Szudzik's Pairing
///       Function definition.
///
/// \see
/// https://en.wikipedia.org/wiki/Pairing_function#Szudzik's_pairing_function
///
template <typename SzudzikIn = std::uint32_t,
          typename SzudzikOut = std::uint64_t>
[[nodiscard]] constexpr static auto szudzik(const SzudzikIn a,
                                            const SzudzikIn b) noexcept
    -> SzudzikOut {
  static_assert(std::numeric_limits<SzudzikIn>::max() *
                        std::numeric_limits<SzudzikIn>::max() <=
                    std::numeric_limits<SzudzikOut>::max(),
                "SzudzikIn type may cause overflow");
  static_assert(is_szudzik_type_v<SzudzikIn>,
                "SzudzikIn type must be an unsigned "
                "integer type but not a "
                "character one.");
  static_assert(is_szudzik_type_v<SzudzikOut>,
                "SzudzikOut type must be an "
                "unsigned integer type but not a "
                "character one.");
  const auto a64 = static_cast<SzudzikOut>(a);
  const auto b64 = static_cast<SzudzikOut>(b);
  return a64 >= b64 ? a64 * a64 + a64 + b64 : a64 + b64 * b64;
}

}; // namespace

class Route {
  /// \brief The path's length.
  ///
  /// This member variable holds the total number of elements in the path of the
  /// route. The length represents the number of elements that make up the route
  /// sequence. It is an essential attribute of the `Route` class, defining the
  /// size of the route.
  ///
  /// \note The length is always non-negative, and it is set based on the number
  ///       of elements stored in the path container.
  ///
  std::size_t m_Length;

  /// \brief The path.
  ///
  /// This member variable represents the route's sequence of elements.
  /// The route is implemented as a dynamic array of type `tw_lpid`.
  /// The `path` array stores the individual elements that make up the route.
  ///
  /// \note The `m_Path` array is dynamically allocated and deallocated as
  ///       needed. It holds `m_Length` elements in the route sequence.
  ///
  /// \warning Accessing `m_Path` directly without considering `m_Length` may
  ///          result in undefined behavior and possible memory access
  ///          violations. In that case, is recommended the use of the `get`
  ///          function.
  std::unique_ptr<tw_lpid *> m_Path;

public:
  /// \brief Constructor for the Route class.
  ///
  /// Creates a new `Route` object with the given path and length.
  ///
  /// \param path A `unique_ptr` to an array of `tw_lpid` values representing
  ///             the path of the route.
  /// \param length The length of the route's path, indicating the number
  ///               of vertices in the route.
  ///
  /// \note The constructor takes ownership of the `path` pointer, transferring
  ///       it to the `m_Path` member using `std::move`. The caller should
  ///       ensure that the memory pointed to by `path` is dynamically allocated
  ///       and manage its deallocation properly. After construction, the
  ///       `Route` object will be responsible for managing the memory and will
  ///       automatically deallocate it when the object is destructed.
  Route(std::unique_ptr<tw_lpid *> path, const std::size_t length)
      : m_Path(std::move(path)), m_Length(length) {}

  /// \brief Access the element at the specified index in the route.
  ///
  /// This function allows users to retrieve the element located at the given
  /// index in the route. The index represents the position of the desired
  /// element in the route, and it is zero-based.
  ///
  /// The route represents a sequence of elements that can be accessed through
  /// index-based addressing. The route is implemented as a collection of
  /// elements stored in the `path` array.
  ///
  /// In debug mode, if the provided index is out of the bounds of the route
  /// (greater than or equal to its length), the program will be immediately
  /// aborted. The `ispd_error` macro provides additional information about the
  /// error, including the invalid index and route length. This ensures that any
  /// potential index overflow issues are detected and addressed during
  /// debugging.
  ///
  /// In release mode, the function works without any exception or overflow
  /// checks, providing optimal performance.
  ///
  /// \param index The index of the element to be accessed in the route.
  /// \return The route's element at the specified index.
  __attribute__((always_inline)) inline auto
  get(const std::size_t index) const noexcept -> tw_lpid {
    DEBUG({
      // Check if the index being accessed will cause an overflow
      // If so, the program is immediately aborted.
      if (index >= m_Length) [[unlikely]]
        ispd_error("Accessing an invalid route element index (Index: %zu, "
                   "Route Length: %zu).",
                   index, m_Length);
    });

    return (*m_Path)[index];
  }

  /// \brief Returns the route's length.
  inline std::size_t getLength() const { return m_Length; }
};

/// \class RoutingTable
///
/// \brief A class representing a routing table to store and manage routes
///        between source and destination vertices.
///
class RoutingTable {
  /// \brief A hash table that stores routes between source and destination
  /// vertices.
  ///
  /// Each route is identified by a unique key, which is calculated based on the
  /// pairing of the source and destination vertices using Szudzik's pairing
  /// function. The key is a 64-bit unsigned integer obtained by applying
  /// Szudzik's pairing function on the source and destination vertex IDs.
  ///
  /// \note The value associated with each key is a constant pointer to the
  ///       corresponding `Route` object, ensuring that the routes themselves
  ///       cannot be modified through this map.
  std::unordered_map<uint64_t, const Route *> m_Routes;

  /// \brief A hash map that keeps track of the number of routes originating
  ///        from each source vertex.
  ///
  /// This map allows an early sanity checking about if the routes from a
  /// speecific vertex match with the specified model built.
  std::unordered_map<tw_lpid, uint32_t> m_RoutesCounting;

  /// \brief Adds a route to the routing table between the given source and
  ///        destination vertices.
  ///
  /// \param src The source vertex (`tw_lpid`) of the route.
  /// \param dest The destination vertex (`tw_lpid`) of the route.
  /// \param route A pointer to a `const Route` object representing the route
  ///              from the source to the destination.
  ///
  /// \note This function uses Szudzik's pairing function to generate a unique
  ///       key for the route based on the source and destination vertices. The
  ///       key is obtained by applying Szudzik's pairing function on the source
  ///       and destination vertex IDs. The route is then stored in the
  ///       `m_Routes` map, indexed by this unique key.
  void addRoute(const tw_lpid src, const tw_lpid dest, const Route *route);

  /// \brief Parses a route line from the input file and extracts the source and
  ///        destination vertices.
  ///
  /// This function is used internally by the `load()` function to read and
  /// parse individual route lines from the input file. It extracts the source
  /// and destination vertices from the route line and returns a dynamically
  /// allocated `Route` object.
  ///
  /// \param routeLine A string containing a single line of the input file
  ///                  representing a route.
  /// \param src A reference to a `tw_lpid` variable that
  ///            will hold the parsed source vertex.
  /// \param dest A reference to a `tw_lpid`
  ///             variable that will hold the parsed destination vertex.
  ///
  /// \returns A pointer to a `Route` object representing the parsed route.
  ///
  /// \note This function expects the input route line to be in a specific
  ///       format, containing source and destination vertex IDs separated by a
  ///       delimiter. It extracts these IDs, converts them to `tw_lpid` format,
  ///       and creates a new `Route` object to represent the route. The caller
  ///       is responsible for managing the memory of the returned `Route`
  ///       object.
  Route *parseRouteLine(const std::string &routeLine, tw_lpid &src,
                        tw_lpid &dest);

public:
  /// \brief Loads route information from the specified file and populates the
  ///        routing table.
  ///
  /// This function reads route data from the input file, parses each route line
  /// using `parseRouteLine()`, and adds the routes to the routing table using
  /// `addRoute()`. It also updates the `m_RoutesCounting` map to keep track of
  /// the number of routes originating from each source vertex.
  ///
  /// \param filepath The path to the input file containing route information.
  ///
  /// \note This function assumes that the input file contains route information
  ///       in a specific format, with each route represented by a single line
  ///       containing source and destination vertex IDs separated by a
  ///       delimiter. The function reads each line, parses it using
  ///       `parseRouteLine()`, and adds the routes to the `m_Routes` map. It
  ///       also updates the `m_RoutesCounting` map to increment the count for
  ///       the corresponding source vertex.
  void load(const std::string &filepath);

  /// \brief Retrieves the route between the specified source and destination
  ///        vertices from the routing table.
  ///
  /// \param src The source vertex (`tw_lpid`) of the desired route.
  /// \param dest The destination vertex (`tw_lpid`) of the desired route.
  ///
  /// \returns A pointer to a `const Route` object representing the route from
  ///          the source to the destination, or `nullptr` if no route is found.
  ///
  /// \note This function uses Szudzik's pairing function to generate the key
  ///       based on the source and destination vertices and looks up the route
  ///       in the `m_Routes` map. It returns the corresponding `Route` object
  ///       if found, or  throws an exception otherwise.
  const Route *getRoute(const tw_lpid src, const tw_lpid dest) const;

  /// \brief Returns the number of routes originating from the specified source
  ///        vertex.
  ///
  /// \param src The source vertex (`tw_lpid`) for which the count of routes is
  ///        desired.
  ///
  /// \returns The count of routes originating from the specified source vertex.
  ///
  /// \note This function retrieves the count of routes for the given source
  ///       vertex from the `m_RoutesCounting` map. It returns the count as an
  ///       unsigned 32-bit integer. This information is useful for sanity
  ///       checking, ensuring that the routes from a specific vertex match the
  ///       expected model built.
  const std::uint32_t countRoutes(const tw_lpid src) const;
};

}; // namespace ispd::routing

namespace ispd::routing_table {

/// \brief Loads route information from the specified file and populates the
///        global routing table.
///
/// This function reads route data from the input file, parses each route line
/// using `parseRouteLine()`, and adds the routes to the routing table using
/// `addRoute()`. It also updates the `m_RoutesCounting` map to keep track of
/// the number of routes originating from each source vertex.
///
/// \param filepath The path to the input file containing route information.
///
/// \note This function assumes that the input file contains route information
///       in a specific format, with each route represented by a single line
///       containing source and destination vertex IDs separated by a
///       delimiter. The function reads each line, parses it using
///       `parseRouteLine()`, and adds the routes to the `m_Routes` map. It
///       also updates the `m_RoutesCounting` map to increment the count for
///       the corresponding source vertex.
void load(const std::string &filepath);

/// \brief Retrieves the route between the specified source and destination
///        vertices from the routing table.
///
/// \param src The source vertex (`tw_lpid`) of the desired route.
/// \param dest The destination vertex (`tw_lpid`) of the desired route.
///
/// \returns A pointer to a `const Route` object representing the route from
///          the source to the destination, or `nullptr` if no route is found.
///
/// \note This function uses Szudzik's pairing function to generate the key
///       based on the source and destination vertices and looks up the route
///       in the `m_Routes` map. It returns the corresponding `Route` object
///       if found, or throws an exception otherwise.
const ispd::routing::Route *getRoute(const tw_lpid src, const tw_lpid dest);

/// \brief Returns the number of routes originating from the specified source
///        vertex.
///
/// \param src The source vertex (`tw_lpid`) for which the count of routes is
///        desired.
///
/// \returns The count of routes originating from the specified source vertex.
///
/// \note This function retrieves the count of routes for the given source
///       vertex from the `m_RoutesCounting` map. It returns the count as an
///       unsigned 32-bit integer. This information is useful for sanity
///       checking, ensuring that the routes from a specific vertex match the
///       expected model built.
const std::uint32_t countRoutes(const tw_lpid src);

}; // namespace ispd::routing_table

#endif // ISPD_ROUTING_HPP
