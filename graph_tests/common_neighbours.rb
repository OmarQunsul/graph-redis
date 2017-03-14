# edges exists

require 'redis'

redis = Redis.new

describe 'Common neighbours' do

  it 'should correctly find the common neighbours between two vertices' do
    redis.flushdb
    redis.gvertex 'graph1', 'a', 'b', 'c', 'd', 'e', 'f'
    redis.gedge 'graph1', 'a', 'c', 1
    redis.gedge 'graph1', 'b', 'c', 1
    redis.gedge 'graph1', 'a', 'd', 1
    redis.gedge 'graph1', 'b', 'd', 1
    redis.gedge 'graph1', 'b', 'e', 1

    common_vertices = redis.gcommon('graph1', 'a', 'b')
    common_vertices.length.should eq 2
    common_vertices.include?('c').should eq true
    common_vertices.include?('d').should eq true
    common_vertices.include?('e').should eq false

    common_vertices = redis.gcommon('graph1', 'a', 'f')
    common_vertices.empty?.should eq true
  end

end

