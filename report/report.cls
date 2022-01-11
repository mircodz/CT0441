\NeedsTeXFormat{LaTeX2e}
\ProvidesClass{report}[2021/01/03, Mirco De Zorzi LaTeX Report Template]
\LoadClass[a4paper,10pt,twoside,twocolumn]{article}

% +--------------------------------------------+
% | Margins                                    |
% +--------------------------------------------+

\RequirePackage[
  top=1cm,      % Top margin
  bottom=1.5cm, % Bottom margin
  left=2cm,     % Left margin
  right=2cm,    % Right margin
  includehead,  % Include space for a header
  includefoot,  % Include space for a footer
]{geometry}

% +--------------------------------------------+
% | Packages                                   |
% +--------------------------------------------+

\RequirePackage{titling}                 % Allows custom title configuration
\RequirePackage[english]{babel}          % English language hyphenation
\RequirePackage{microtype}               % Better typography
\RequirePackage{amsmath,amsfonts,amsthm} % Math packages for equations
\RequirePackage[svgnames]{xcolor}        % Enabling colors by their 'svgnames'
\RequirePackage{lastpage}                % Used to determine the number of pages in the document (for "Page X of Total")
\RequirePackage{graphicx}                % Required for adding images
\RequirePackage{hyperref}                % Hyperlinks

\RequirePackage{enumitem}
\setlist{noitemsep} % Remove spacing between bullet/numbered list elements

% +--------------------------------------------+
% | Font                                       |
% +--------------------------------------------+

\RequirePackage[T1]{fontenc}    % Output font encoding for international characters
\RequirePackage[utf8]{inputenc} % Required for inputting international characters
\RequirePackage[sc]{mathpazo}
\linespread{1.05}
\RequirePackage{microtype}

% +--------------------------------------------+
% | Header                                     |
% +--------------------------------------------+

\RequirePackage{fancyhdr} % Needed to define custom headers/footers
\pagestyle{fancy} % Enables the custom headers/footers

\renewcommand{\headrulewidth}{0.0pt} % No header rule
\renewcommand{\footrulewidth}{0.0pt} % No footer rule

\renewcommand{\sectionmark}[1]{\markboth{#1}{}} % Removes the section number from the header when \leftmark is used

\fancyhf{}
\cfoot{\footnotesize Page \thepage~of~\pageref{LastPage}} % Right footer, "Page 1 of 2"
\fancypagestyle{plain}{
  \cfoot{\footnotesize Page \thepage~of~\pageref{LastPage}} % Right footer, "Page 1 of 2"
}

% +--------------------------------------------+
% | Minted                                     |
% +--------------------------------------------+

\RequirePackage{minted}
\AtBeginEnvironment{minted}{%
  \renewcommand{\fcolorbox}[4][]{#4}}
