% Generated by roxygen2: do not edit by hand
% Please edit documentation in R/plot.lasmetrics.r
\name{plot.lasmetrics}
\alias{plot.lasmetrics}
\title{Plot an object of class lasmetrics in 2D}
\usage{
\method{plot}{lasmetrics}(x, z = NULL, colorPalette = height.colors(50),
  ...)
}
\arguments{
\item{x}{A data.frame or data.table of class \code{lasmetrics}.}

\item{z}{character. The field to plot. If NULL, autodetect.}

\item{colorPalette}{characters. a list of colors such as that generated by heat.colors,
topo.colors, terrain.colors or similar functions.  Default is \code{height.colors(50)}
provided by the package \code{lidR}}

\item{\dots}{Supplementary parameters for \link[raster:plot]{plot}}
}
\description{
This functions implements a plot method for a \code{lasmetrics} data.frame
}
\details{
The \dots param provides additional arguments to \link[raster:plot]{plot}.
}
\examples{
LASfile <- system.file("extdata", "Megaplot.laz", package="lidR")
lidar = readLAS(LASfile)

# Canopy surface model with 4 m^2 cells
grid_canopy(lidar) \%>\% plot

# Mean height with 400 m^2 cells
grid_metrics(lidar, mean(Z)) \%>\% plot

# Define your own metric function
myMetrics = function(z, i, angle, pulseID)
{
  ret = list(
        npulse  = length(unique(pulseID)),
        hmean   = mean(z),
        hmax    = max(z),
        imean   = mean(i),
        angle   = mean(abs(angle))
        )

   return(ret)
 }

metrics = grid_metrics(lidar, myMetrics(Z, Intensity, ScanAngle, pulseID))

plot(metrics, "hmean")
plot(metrics, "hmax")
plot(metrics, "imean")
}
\seealso{
\link[lidR:grid_metrics]{grid_metrics}
\link[lidR:grid_canopy]{grid_canopy}
\link[lidR:height.colors]{height.colors}
\link[lidR:forest.colors]{forest.colors}
\link[grDevices:heat.colors]{heat.colors}
\link[grDevices:colorRamp]{colorRampPalette}
}
