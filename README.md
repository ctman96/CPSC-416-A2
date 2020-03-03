<div class="description user_content student-version enhanced"><p>Updates:</p>
<p>February 12 - Changed requirement to list repos to the name of the person you are testing with as opposed to the repo name which includes your CWL.</p>
<p>IMPORTANT: This assignment may be done in pairs and you are strongly encouraged to do that. Late assignments penalized at 33.333% per day pro-rated. Late assignments not accepted after 2 days</p>
<p>In this assignment you will implement the vector time-stamp algorithm to trace runs of the bully election algorithm, which you will also implement.&nbsp; Tracing consists of writing specially formatted lines to a log file.&nbsp; These log files can then be used to visualize the message exchanges and event orderings between the various "nodes." To visualize the bully algorithm you will use the software package <a href="http://bestchai.bitbucket.io/shiviz/" class="external" target="_blank" rel="noreferrer noopener"><span>ShiViz</span><span aria-hidden="true" class="ui-icon ui-icon-extlink ui-icon-inline" title="Links to an external site."></span><span class="screenreader-only">&nbsp;(Links to an external site.)</span></a> developed by Ivan Beschastnikh and his students here at UBC.</p>
<p>To use ShiViz you need to produce a log file with events tagged with a vector time-stamp. Although the tool is quite configurable, for this assignment an event consists of two or more lines. If there are N lines to an event, then the first N-1 lines are some sort of description of the event, and the last line is the vector time-stamp for that event. For this assignment you will probably want to use two lines, one for the event description and then the vector time-stamp line.</p>
<p>The vector time-stamp rules used by ShiViz are a bit different from those discussed in class. In class we only incremented a node's clock just before it sent a message. It was noted that we could actually increase the clock at anytime we wanted since we could imagine a node whose role was unimportant to our algorithm as the recipient of a message every time we wanted to update the clock. Given that, the rules to use for the clock update and logging are as follows:</p>
<ol>
<li>Increment the node's clock just before a send, log the send and then perform the send.</li>
<li>Merge the local time vector with the vector arriving with the message, update (this is different) the local clock (note when merging you ignore the time for your local clock within the vector, but you probably want to check that it is less than or equal to your clock, if it isn't then there is a bug in your code), and then log the receipt of the message.</li>
<li>For any other event you want to log (e.g. a node decides it is co-coordinator, a node decides to call an election, etc) increment the clock and then log the event. (This avoids having to treat events on the same node in a special way.)</li>
<li>Logging an event consists of writing a line describing the event, followed by the vector time-stamp on the next line. The line just before the vector time-stamp is used to tag the event in ShiViz so it should be reasonably descriptive and provide sufficient information so that you can relate the events back to what your code is doing.</li>
</ol>
<h3>The Log File</h3>
<p>Each "node" is to keep its own event log file. Here are some examples of the log file format you need to produce for ShiViz:</p>
<pre>Starting N1
N1 {"N1":1}
Send Election MSG  to N2
N1 {"N1" : 2}
Send Election MSG to N3
N1 {"N1" : 3}
Receive Answer
N1 {"N1" :4 , "N2" : 3}
Receive Answer
N1 {"N1": 5, "N2" : 3, "N3" : 3}
Receive COORD <br>N1 {"N1" :6, "N2" : 4, "N3" : 8}
</pre>
<p>In the above example, each two lines are an event. The first line of the two is some sort of description of the event and is free format. The 2nd line is the vector time of the event. Each vector time line starts with the identifier of the node. The time then consists of JSON tuples identifying the time at all known nodes. A known node is one that a clock value has been received for either directly or transitively. You don't include nodes that are supposed to exist but you don't know the time of. Also observe that the initial clock value at a node is 1. The time vector is enclosed between "{" and "}". Each time value consists of the node identifier in quotes, followed by a ":" and then the time. Individual times are separated using a ",".</p>
<p>For ShiViz to work it needs to know the events from all the nodes. Here are the two log files for nodes N2 and N3 that need to be combined with the above log file to allow ShiViz to diagram things out.</p>
<p>N2's Log file</p>
<pre>Starting N2
N2 {"N2" : 1}
Receive Election MSG
N2 { "N1" : 2,  "N2" : 2}
Send Answer 
N2 {"N1" :2,  "N2" :3}
Send Election GG
N2 {"N1" : 2, "N2" :4}
Receive Answer GG
N2 {"N1" : 2, "N2" : 5, "N3" : 5}
Receive COORD<br>N2 {"N1" :3, "N2" : 6, "N3" : 7}
</pre>
<p>N3's log file</p>
<pre>Starting N3
N3 {"N3" : 1}
Receive Election MSG
N3 { "N1" : 3,  "N3" : 2}
Send Answer 
N3 {"N1" :3 , "N3" :3}
Receive Election MSG GG
N3 {"N1" :3 , "N2" : 4, "N3" :4}
Send Answer  GG
N3 {"N1" :3 , "N2" : 4, "N3" :5}<br>Declare self Coordinator<br>N3 {"N1" :3 , "N2" : 4, "N3" :6}
Send Coord To N2
N3 {"N1" :3, "N2" : 4, "N3" : 7}<br>Send Coord to N1<br>N3 {"N1" :3, "N2" : 4, "N3" : 8}
</pre>
<p>To try ShiViz follow this link: <a href="https://bestchai.bitbucket.io/shiviz/" class="external" target="_blank" rel="noreferrer noopener"><span>http://bestchai.bitbucket.io/shiviz/</span><span aria-hidden="true" class="ui-icon ui-icon-extlink ui-icon-inline" title="Links to an external site."></span><span class="screenreader-only">&nbsp;(Links to an external site.)</span></a> and then click the "Try out ShiViz" button. Cut and past the above log files, one after the other, into the text box on the right. You then need to enter into the "Log parsing regular expression:" the regular expression shown below.</p>
<pre>(?&lt;event&gt;.*)\n(?&lt;host&gt;\S*) (?&lt;clock&gt;{.*})
</pre>
<p>Click on visualize to see a graph of the events and message exchanges. Instead of cutting and pasting you could also cat the log files together and then upload the file. You are strongly encouraged to explore ShiViz and the log file format to get a better understanding of the type of information you want to use to name/describe the event and how the vector clock values need to change, and what to log to provide so that the resulting diagram provides the most useful information. A slightly more elaborate log file has been provided in the repo for this assignment. Note: These log files are for illustrative purposes and do not match what needs to be produced for the assignment.</p>
<h2>Election Algorithm and Vector Clocks</h2>
<p>You are to program in C, the bully algorithm using UDP and, with each message sent as part of the algorithm, you are to include the vector time stamp (see the files in the repo).</p>
<p>Your program is to be named node and take 6 parameters. The program must compile and run on the Linux machines provided by the department. Having a solution that works/compiles on a different machine but not work on the department's Linux machines will be awarded 0 for all code related marks, so make sure it works. The 6 parameters the program is to take, in the listed order are:</p>
<ol>
<li>
<strong>PortNumber</strong> The port number this node will listen on. If this port number cannot be found in the group list your program is to print an error message and exit.</li>
<li>
<strong>GroupListFileName</strong> The name of the file containing the list of all the nodes that are part of this "election" group. if this parameter is simple a minus sign "-", then you are to read the group list from standard input.</li>
<li>
<strong>LogFileName</strong>The name of the file to write the log events to. If the file already exists you are to truncate the file to size 0 before writing to it. This can be done with the O\_TRUNC option on open.</li>
<li>
<strong>Timeout Value</strong> The timeout value is an integer value , in seconds, for all timeout related events in your program.</li>
<li>
<strong>AYA time</strong>This is the average amount of time that a node waits before sending an AYA message to the coordinator. See the sample code for an example of how to use this.</li>
<li>
<strong>SendFailure</strong> The probability that a sent packet will not arrive. The value is an integer in the range 0 to 99,&nbsp; inclusive, with 0 meaning no packets are lost.</li>
</ol>
<p>The above parameters are not optional and must always be provided. If a number parameter cannot be converted to a number an error is to be reported and the program is to exit. With respect to file names, if a file needs to be opened and cannot, then an error is to be reported and the program is to exit.</p>
<p>If you want you may add additional parameters after these to aid in program development and debugging. However these parameters must be optional such that if the program is run without the optional parameters it behaves as required.</p>
<h3>Group Management</h3>
<p>One of the big challenges with the election algorithms is group management. For this assignment we are going to simplify that aspect of things by having a "node" read the list of the group members from a file. The file will consist of one or more lines formatted as follows:</p>
<pre>  hostname portNo
</pre>
<p>Where nodeNumber is an integer value that is the node's number, the hostname is the hostname or IP address where that node resides, and the port number is the port the node is listening for UDP packets on. An example file might be something like this:</p>
<pre>localhost 1856
localhost 1857
remote.ugrad.cs.ubc.ca 2000
</pre>
<p>On starting the node will read the above file and build the list of group members. A node's ID will be its port number. (When printing a node number in a log put an N in front. For example N1856) Since the port number is the Node ID, this implies that the port numbers have to be unique regardless of what computer the program is running on. You may assume that all lines are properly formatted and that the various fields are separated by a single space and that there are no leading blanks on a line. Your program, however, is to exit gracefully with an error message if the file cannot be opened for reading. Your program must be able to handle 8 nodes plus itself. For vector time-stamp purposes you are to assume that the vector clocks sent in messages all have 9 entries, although some of them may be unused. Since every node will have its own ID (i.e. port number) it is important for a node to know which ID it is associated with. That information is passed on the command line.&nbsp; Consequently, if the port number on the command line cannot be found in the group management file there is a problem and the program is to print an error message and exit gracefully.</p>
<p>In the repo, the file msg.h contains the definition of the message to be sent along with the definitions of the message types.</p>
<h3>Reliable Delivery</h3>
<p>Although UDP is an unreliable protocol in most situations on a local area network,&nbsp; or a collection of processes of the same machine, packets get through. To simplify things you are to assume that all UDP packets sent are received, even though that may not be the case. (If you implement the algorithm properly this won't matter, and in the worst case an election will be called when one wasn't needed.)</p>
<p>To simulate network problems you are to use the SendFailure parameter to determine if the packet should actually be sent or just dropped. If the value is 0 then no packets are dropped. To determine if a packet is to be dropped generate a random number from 0 to 99 inclusive and if the number is less than the SendFailure value then the packet is dropped, otherwise it is sent.</p>
<h3>Interoperability</h3>
<p>Your program must interoperate with other implementations in the class. Ideally everyone's program should interoperate with everyone else's but implementing that level of testing is a bit problematic. Consequently you are required to test your program against at least one other implementation, but more is better. In the file Report.txt you are to include the names of the authors of the other program or programs you tested against and the success or failure of your interoperability. You must also include descriptions of the test scenarios you used.&nbsp; If the interoperability test(s) failed you are to&nbsp; attempt to resolve and fix the problem and report that the outcome of those attempts. In some cases the fix may be very time consuming in which case just report the problem, why the problem exists, and propose a solution. A simple report that encountered no problems would be on the order of 150 words.</p>
<h3>ShiViz</h3>
<p>Since ShiViz is a tool for vitalizing interactions in a distributed system one of the first tasks you undertake should be to make sure your process can produce a log file that can be displayed by ShiViz. Then, as you develop and debug your implementation, you can use ShiViz to visualize your log files. (To do this, you will need to take the log files from the processes involved in the test run and cat them together to produce one file. You can then upload that file to ShiViz.) As part of this assignment you are to submit an example of one of the log files you used with ShiViz (it is to be named ShiViz-Log.dat). The log file can show a particularly interesting election example or can be an example of how you used ShiViz to debug your program. In the file ShiViz-Report.txt you are to provide a brief write-up describing the scenario corresponding to your log file and why this run is interesting, be it the nature of the interactions or how you used ShiViz in this case to help you debug your code. The write-up is to be be in clear grammatically correct English (make sure to run a spell-checker on the submission) of between 200 and 300 words. To give you a sense of size, this paragraph is about 230 words.</p>
<h3>Starter code</h3>
<p>Follow the instructions <a title="Assignment Policies and Procedures" href="/courses/36019/pages/assignment-policies-and-procedures" data-api-endpoint="https://canvas.ubc.ca/api/v1/courses/36019/pages/assignment-policies-and-procedures" data-api-returntype="Page">here</a> to get your repo and to review the instructions that apply to assignments in this course.&nbsp; A simple node.c program is provided to give you with some guidance on how to proceed along with some sample code on how to make some of the library calls need for the sending and receiving of UDP packets.</p>
<p>&nbsp;</p>
<h3>What to hand in.</h3>
<p>All work is to be handed in via stash. Do not, any any circumstances hand-in object code, executable files, or any other form of binary file and that includes word or PF documents. Make sure you hand-in:</p>
<ol>
<li>A working Make-file that will compile your program on the department's Linux machines and produce an executable program called node.</li>
<li>All the .c and .h files required to compile your program. Your code is to be well commented and appropriately formatted.</li>
<li>The coverpage.txt file. See the contents of the file for an explanation of how to hand it in.</li>
<li>Report.txt - your report on interoperability.</li>
<li>ShiVizReport.txt - Your report describing the log data.</li>
</ol>
<p>&nbsp;</p>
<h3>Grading</h3>
<p>Here is a rough grading guideline. This is meant to provide you with some guidance, but I do not guarantee that that final grading rubric will match this mark distribution exactly, but it should be close.</p>
<ol>
<li>Implementation of the Bully Algorithm 25%</li>
<li>Implementation of Vector clocks 15%</li>
<li>Logging to a file in ShiViz acceptable format 10%</li>
<li>Interoperability 10%</li>
<li>ShiViz example 15%</li>
<li>Interview with TA 25% (Tentative, if not done the marks will be redistributed to other items)</li>
</ol></div>
