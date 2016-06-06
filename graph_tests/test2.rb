# Minimum spanning tree

require 'redis'

redis = Redis.new

describe 'Minimum spanning tree' do

  it 'should calculate the correct minimum spanning tree' do
    redis.flushdb
    redis.gvertex 'graph1', 'a', 'b', 'c', 'd', 'e'
    redis.gedge 'graph1', 'a', 'b', 1
    redis.gedge 'graph1', 'c', 'b', 1
    redis.gedge 'graph1', 'a', 'd', 100
    redis.gedge 'graph1', 'd', 'e', 1
    redis.gedge 'graph1', 'a', 'e', 3
    redis.gedge 'graph1', 'd', 'c', 1

    redis.gmintree('graph1', 'graph2')
    min_tree = redis.gedges('graph2')

    min_tree.length.should eq 4 * 3
  end

end

