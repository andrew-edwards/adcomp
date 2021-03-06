% Generated by roxygen2: do not edit by hand
% Please edit documentation in R/sdreport.R
\name{as.list.sdreport}
\alias{as.list.sdreport}
\title{Convert estimates to original list format.}
\usage{
\method{as.list}{sdreport}(x, what = "", ...)
}
\arguments{
\item{x}{Output from \code{\link{sdreport}}.}

\item{what}{Select what to convert.}

\item{...}{Passed to \code{\link{summary.sdreport}}.}
}
\value{
List of same shape as original parameter list.
}
\description{
Get estimated parameters or standard errors in the same shape as
the original parameter list.
}
\details{
This function converts the selected column \code{what} of
\code{summary(x, select = c("fixed", "random"), ...)} to the same
format as the original parameter list (re-ordered as the template
parameter order). The argument \code{what} is partially matched
among the column names of the summary table. The actual match is
added as an attribute to the output.
}
\examples{
\dontrun{
example(sdreport)
as.list(rep, "Est")
as.list(rep, "Std")
as.list(rep, "Pr", p.value=TRUE)
}
}

