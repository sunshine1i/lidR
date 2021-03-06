% Generated by roxygen2: do not edit by hand
% Please edit documentation in R/tree_metrics.r
\name{tree_metrics}
\alias{tree_metrics}
\title{Compute metrics for each tree}
\usage{
tree_metrics(.las, func, debug = FALSE)
}
\arguments{
\item{.las}{An object of class \code{LAS}}

\item{func}{the function to be applied to each tree}

\item{debug}{logical. If you encouter a non trivial error try \code{debug = TRUE}.}
}
\value{
It returns a \code{data.table} containing the metrics for each tree The table
has the class "lastreemetrics" enabling easy plotting.
}
\description{
Once the tree are segmented, computes a series of descriptive statistics defined by the user
for a LiDAR dataset for each tree. Output is a data.frame in which each line is a tree
(single grid cell)
}
\details{
Computes a series of descriptive statistics defined by the user. Output is a
data.frame in which each line is a tree, and each column is a metric.
tree_metrics is similar to \link{lasmetrics} or \link{grid_metrics} or \link{grid_metrics3d}
or \link{grid_hexametrics} except it computes metrics for each tree. The following existing
function can help the user to compute some metrics:
\itemize{
\item{\link[lidR:stdmetrics]{stdmetrics_tree}}
} Users must write their own functions to create metrics. \code{tree_metrics} will
dispatch the LiDAR data for each cell in the user's function. The user writes their
function without considering grid cells, only a cloud of points (see example).
}
\examples{
LASfile <- system.file("extdata", "Tree.laz", package="lidR")
las = readLAS(LASfile, Classification = FALSE, filter = "-drop_z_below 0")

# segment tree (refer to lastrees)
lastrees(las, algorithm = "li2012")

# Max height for each tree
tree_metrics(las, mean(Z))

# Define your own new metrics
myMetrics = function(z)
{
  metrics = list(
     zmean = mean(z),
     zmax  = max(z),
     npoint = length(z)
   )

   return(metrics)
}

metrics = tree_metrics(las, myMetrics(Intensity))

# predefined metrics (see ?stdmetric)
metrics = tree_metrics(las, .stdtreemetrics)
}
