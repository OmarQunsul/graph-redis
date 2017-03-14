# Directed Graphs

require 'redis'

redis = Redis.new

describe 'Minimum spanning tree' do
  it 'should calculate the correct minimum spanning tree' do
    redis.flushdb
    redis.gvertex 'graph1', 'a', 'b', 'c', 'd'
    redis.gsetdirected 'graph1'
    redis.gedge 'graph1', 'a', 'b', 1
    redis.gedge 'graph1', 'b', 'a', 4
    redis.gedge 'graph1', 'b', 'c', 2
    redis.gedge 'graph1', 'c', 'b', 3
    redis.gedge 'graph1', 'c', 'd', 3

    path1 = redis.gshortestpath 'graph1', 'a', 'c'
    path2 = redis.gshortestpath 'graph1', 'c', 'a'
    path3 = redis.gshortestpath 'graph1', 'a', 'd'
    path1.last.should eq '3'
    path2.last.should eq '7'
    path3.last.should eq '6'

    redis.gneighbours('graph1', 'c').should eq ['b', 'd']
    redis.gneighbours('graph1', 'd').should eq []
    redis.gincoming('graph1', 'd').should eq ['c']
  end
end

