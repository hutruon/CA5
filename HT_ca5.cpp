#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <queue>

using namespace std;

struct Actor
{
    string name;
    unordered_set<string> knownForTitles;
};

struct Title
{
    string titleId;
    string primaryTitle;
};

struct Graph
{
    unordered_map<string, unordered_map<string, string>> adjList; // Actor -> (Co-actor -> Movie Title ID)
};

void addEdge(Graph &graph, const string &actor1, const string &actor2, const string &movieTitleId)
{
    graph.adjList[actor1][actor2] = movieTitleId;
    graph.adjList[actor2][actor1] = movieTitleId;
}

string trim(const string &str)
{
    size_t first = str.find_first_not_of(' ');
    if (string::npos == first)
    {
        return str;
    }
    size_t last = str.find_last_not_of(' ');
    return str.substr(first, (last - first + 1));
}

Actor parseActorLine(const string &line)
{
    istringstream iss(line);
    string token;
    Actor actor;

    getline(iss, token, '\t');      // Skip the first column (nconst)
    getline(iss, actor.name, '\t'); // Parse primaryName (second column)

    // Skip columns until knownForTitles (sixth column)
    for (int i = 0; i < 3; ++i)
    {
        getline(iss, token, '\t');
    }

    string titles;
    getline(iss, titles, '\t'); // Get knownForTitles
    istringstream titlesStream(titles);
    while (getline(titlesStream, token, ','))
    {
        actor.knownForTitles.insert(trim(token)); // Use trim here
    }

    return actor;
}

Title parseTitleLine(const string &line)
{
    istringstream iss(line);
    Title title;
    string token;

    getline(iss, title.titleId, '\t'); // Parse titleId
    // Skip the next columns until the primaryTitle
    getline(iss, token, '\t');              // Skip titleType
    getline(iss, title.primaryTitle, '\t'); // Parse primaryTitle

    // Trim the titleId to remove any extra spaces or characters
    title.titleId = trim(title.titleId);

    return title;
}

// BFS Function to find the shortest path between actors
vector<pair<string, string>> bfs(const Graph &graph, const string &startActor, const string &endActor)
{
    if (startActor == endActor)
    {
        return {}; // Empty path for the same actor
    }

    queue<string> q;
    unordered_set<string> visited;
    unordered_map<string, pair<string, string>> parent; // Actor -> (Parent Actor, Movie Title ID)

    q.push(startActor);
    visited.insert(startActor);

    while (!q.empty())
    {
        string current = q.front();
        q.pop();

        if (current == endActor)
        {
            vector<pair<string, string>> path;
            while (current != startActor)
            {
                string parentActor = parent[current].first;
                if (graph.adjList.at(parentActor).find(current) != graph.adjList.at(parentActor).end())
                {
                    string movieId = graph.adjList.at(parentActor).at(current); // Corrected using at()

                    path.push_back({current, movieId});
                }
                else
                {
                    // Debug print if the movie ID is not found
                    cout << "Debug - Movie ID not found for actors: " << parentActor << " and " << current << endl;
                }
                current = parentActor;
            }
            path.push_back({startActor, ""});
            reverse(path.begin(), path.end());
            return path;
        }

        for (const auto &neighbor : graph.adjList.at(current))
        {
            if (visited.find(neighbor.first) == visited.end())
            {
                visited.insert(neighbor.first);
                parent[neighbor.first] = make_pair(current, neighbor.second); // Store the movie ID connecting the actors
                q.push(neighbor.first);
            }
        }
    }

    return {}; // If no path found, return an empty vector
}

void printPathAndDegree(const vector<pair<string, string>> &path, const unordered_map<string, Title> &titles)
{
    if (path.empty())
    {
        cout << "No connection found." << endl;
        return;
    }

    int degree = path.size() - 1; // Degree is the number of actors in the path minus 1 (Helena herself)
    cout << ">> The degree of separation between " << path.front().first << " and " << path.back().first << " is " << degree << ". " << endl;

    for (size_t i = 0; i < path.size() - 1; ++i)
    {
        string actorName = path[i].first;
        string nextActorName = path[i + 1].first;
        string movieTitleId = path[i + 1].second; // Movie ID connecting to the next actor

        // Make sure we find the movie title in the titles map
        if (titles.find(movieTitleId) != titles.end())
        {
            cout << ">> " << actorName << " has starred with " << nextActorName
                 << " in the movie " << titles.at(movieTitleId).primaryTitle;
            cout << "." << endl;
        }
        else
        {
            cout << ">> " << actorName << " has an unknown title connection with " << nextActorName << "." << endl;
        }
    }
}

int main()
{
    ifstream file("names.basics.tsv");
    if (!file.is_open())
    {
        cerr << "Failed to open file for actors\n";
        return 1;
    }

    unordered_map<string, unordered_set<string>> titleToActors;
    string line;
    getline(file, line); // Skip header line

    while (getline(file, line))
    {
        Actor actor = parseActorLine(line);
        for (const auto &title : actor.knownForTitles)
        {
            titleToActors[title].insert(actor.name);
        }
    }
    file.close();

    unordered_map<string, Title> titles;
    file.open("titles.basics.tsv");
    if (!file.is_open())
    {
        cerr << "Failed to open file for titles\n";
        return 1;
    }

    getline(file, line); // Skip header line
    while (getline(file, line))
    {
        Title title = parseTitleLine(line);
        titles[trim(title.titleId)] = title;
    }
    file.close();

    Graph graph;
    for (const auto &pair : titleToActors)
    {
        const auto &actors = pair.second;
        const auto &titleId = pair.first;
        for (auto it = actors.begin(); it != actors.end(); ++it)
        {
            for (auto it2 = next(it); it2 != actors.end(); ++it2)
            {
                addEdge(graph, *it, *it2, titleId);
            }
        }
    }

    string startActor = "Helena Bonham Carter";
    string endActor;
    cout << "Enter the name of an actor to find their degree of separation from " << startActor << ": ";
    getline(cin, endActor);

    vector<pair<string, string>> path = bfs(graph, startActor, endActor);
    printPathAndDegree(path, titles);

    return 0;
}
