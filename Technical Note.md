# Technical Note

## Design Decisions

This project is implemented using a modular structure. The system is divided into several components such as **Agent**, **Pipeline**, **ArxivClient**, and **Reporter**. E
ach component is responsible for a specific task, which makes the code easier to organize and maintain.

The system follows a pipeline-style workflow. The Agent receives the user query and then calls the Pipeline to execute the main process. The Pipeline handles fetching papers from arXiv, removing duplicates, computing relevance scores, and selecting the most relevant papers. Finally, the Reporter generates the output files.

Concurrency is implemented using `std::async`. Each paper is processed independently when computing relevance scores and generating summaries. A configurable worker limit is used to control how many tasks run in parallel. This allows multiple papers to be processed at the same time while keeping the implementation simple.

The relevance score is calculated using a simple keyword-based method. The program checks whether query keywords appear in the paper title or abstract. Matches in the title receive higher scores than matches in the abstract. After scoring, papers are sorted by their relevance score and the top 5 papers are selected.

## Trade-offs

The keyword-based scoring method is simple and efficient, and it does not require any external machine learning libraries. However, it cannot fully understand the semantic meaning of the text. Papers that are related but use different wording may receive a lower score.

Using `std::async` simplifies concurrency compared to manually managing threads. However, it provides less control compared to a more advanced thread pool implementation.

To keep the generated report readable, only a short preview of the abstract is included in the summary instead of the full abstract. This makes the report easier to read, but some detailed information from the full abstract is not shown.