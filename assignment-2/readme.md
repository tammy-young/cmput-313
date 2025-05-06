# - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Name : Tammy Young
# SID : 1706229
# CCID : tqyoung
# - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

# Part 2 Questions

## Part 2 - 1
When using the `reno` TCP congestion algorithm for a buffer size of 10 packets, the mean webpage fetch time was 7.000s and the standard deviation was 1.680s. When the buffer size was increased to 50 packets, the mean webpage fetch time increased to 10.725s and the standard deviation decreased to 0.991s.

## Part 2 - 2
Comparing the Ping RTT measurements in `data_reno_10/data_reno_10.png` and `data_reno_50/data_reno_50.png`, we observe that the RTT when the buffer size is 10 is 121.702ms +- 26.054ms with much more fluctuations/volatility, whereas the RTT when the buffer size is 50 is 467.612ms +- 118.187ms with much less fluctuations/volatility.

The smaller buffer size of 10 impacts RTT by making it fluctuate more and have a lower average (in comparison to RTT when the buffer size is 50) because with the congestion algorithm `reno`, there are fast retransmissions and fast recoveries. Since the buffer size is small (10), the queue becomes full quicker, so packets are dropped more often but are retransmitted quickly, causing frequent fluctuations/oscillations in the graph and shorter RTTs.

The larger buffer size of 50 impacts RTT by making it fluctuate less and have a higher average (in comparison to RTT when buffer size is 10) because a larger buffer means the packets can sit in the buffer for longer times. The few fluctuations that occur in `data_reno_50/data_reno_50.png` represent packets being dropped when the buffer is full. It is then retransmitted quickly as per the `reno` congestion algorithm.

## Part 2 - 3
In terms of webpage fetch time (a measure of throuput), packet loss in `reno` causes aggressive reduction of the congestion window, leading to lower throughput and longer webpage fetch times. `cubic` uses a cubic function to grow the congestion window, so there is faster recovery after packet loss since the congestion window is larger. This allows for higher throughput and shorter webpage fetch times.

In terms of ping RTT, using `reno` prevents long queuing times in buffers because of the fast reaction to packet loss by quickly retransmitting the packet. This lowers the latency, which lowers average RTT. The cubic function used in `cubic` for increasing the congestion window size can lead to bufferbloat, which increases queueing delays and leads to an overall increase in RTT.

If the link betwen the router and h2 had much higher bandwidth, we would see improvement using the `cubic` congestion algorithm, but none using `reno`. `reno`'s conservative congestion window growth acts as a bottleneck, so the increased bandwidth would not really improve throughput or latency, and thus webpage fetch time and ping RTT would not really be affected. `cubic` follows a cubic function for increase congestion window size, so it is designed to handle high-bandwidth links/networks. The throughput would increase and thus the webpage fetch time would decrease. However, the increased bandwidth means more packets arriving in the buffers, which could cause bufferbloat and higher ping RTT.

## Part 2 - 4
From my experiments, considering the buffer size of 50 packets, `bbr` has lower average latency and higher throughput than `cubic` and `reno`. Looking at `data_bbr_50/data_bbr_50.png`, we observe that the average ping RTT is 306.305ms +- 163.744ms, in comparison to `data_cubic_50/data_cubic_50.png` and `data_reno_50/data_reno_50.png`, which have average ping RTT of 502.807ms +- 119.518ms and 467.612ms +- 118.187ms, respectively. Since ping RTT is a measure of latency, these charts show that `bbr` has lower average latency than `cubic` and `reno`. We also observe that webpage fetches take 10.725s +- 0.991s for `reno`, 11.851s +- 1.777s for `cubic`, but only 4.937s +- 1.157s for `bbr`. This shows that the average webpage fetch times are lower for `bbr`, and since webpage fetch time is a measure of throughput, we can conclude that `bbr` has higher throughput than `cubic` and `reno`.

The congestion control algorithm that can better mitigate the bufferbloat problem is `bbr`. This is because `bbr` uses bottleneck bandwidth and RTT to adjust the packet sending rate, avoiding excessive queueing in buffers. Bufferbloat is when there is excessive queueing in buffers causing long queuing delays. By regulating the sending rate so that queues don't build up, it helps to mitigate the bufferbloat problem. `cubic` uses a cubic function for increasing the congestion window size, which could cause excessive queuing in buffers (bufferbloat). Additionally, with the aggressive reduction in cwnd while using `reno`, there is lower throughput, making `bbr` the best for mitigating the bufferbloat problem.

## Part 2 - 5
Increasing buffer size reduces performance and causes bufferbloat because increasing the buffer size means that more packets can queue before the buffer overflows longer, increasing queueing times. Comparing buffer sizes 10 and 50 for the `cubic` congestion algorithm for example, we observe that when the buffer size is 50 (results in `data_cubic_50/data_cubic_50.png`), there are significantly long RTTs and longer curl times, 502.807ms +- 119.518ms and around 11.851s +- 1.777s, respectively. When the buffer size is 10 (results in `data_cubic_10/data_cubic_10.png`), there are shorter RTTs and shorter curl times, 128.645ms +- 21.141ms and 6.908s +- 1.203s, respectively. One of the signals that TCP congestion control algorithms rely on are the packet loss signal. With larger buffers, packets can sit in buffers for longer times before the buffer overflows. TCP congestion control algorithms would interpret no packet loss signal (since the packet is still in the queue) as a sign of success and that there's available bandwidth. With a large buffer size, there will be a delay in congestion detection because of the lack of a packet loss signal, which could cause over-queuing and bufferbloat, leading to increased queueing delays, overall increase in latency, and reduction in performance.

# Part 3 Questions

## Part 3 - 1
In part 2 where there was no packet loss, `reno` and `cubic` expand cwnd aggressively, and they become limited only by queuing delay and buffer size. In terms of queue length, the queue fills consistently because of the stream of packets coming in. This results in the congestion window growing steadily, but also when there's packet loss, there's aggressive reduction of the congestion window. `reno` accounts for the bottleneck bandwidth and RTT to adjust packet sending rate, preventing excessive queueing in buffers. In part 3, there is loss introduced to the links. In my experiment, I applied 5% loss on the links. `reno` and `cubic` respond to loss by reducing the congestion window size. More packets sent also means more packets lost, leading to more fluctuations in cwnd (as observed in `lossy_reno_10/lossy_reno_10.png`, `lossy_reno_50/lossy_reno_50.png`, `lossy_cubic_10/lossy_cubic_10.png`, or `lossy_cubic_50/lossy_cubic_50.png`). The queue length decreased because a packet loss signal is sent to the network congestion algorithm, and it responds by reducing the congestion window size, leading to a decrease in queue length. `bbr` does not depend on packet loss signals, and just works to adjust packet sending rate with bottleneck bandwidth and RTT, preventing excessive queueing in buffers.

## Part 3 - 2
When the buffer size is 50 packets, I think the most performant congestion control algorithm, in terms of throughput, in the presence of increased random loss is `bbr`. I think this is the case because unlike `reno` and `cubic`, `bbr` doesn't interpret loss signals as a sign of congestion. Instead, it uses bottleneck bandwidth and RTT to adjust the packet sending rate, which helps with avoiding significant reductions in cwnd and with balancing bottlenecking with packet sending rate. This balance maximizes throughput.

On the other hand, `reno` and `cubic` both interpret the packet loss signal as a sign of congestion, and in response, aggressively reduces cwnd. This process leads to an underutilization of link bandwidth, decreasing the throughput and making them less performant in this aspect compared to `bbr`.
