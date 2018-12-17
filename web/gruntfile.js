module.exports = function( grunt ) {

   grunt.loadNpmTasks( 'grunt-contrib-concat' );

   grunt.initConfig( {
      concat: {
         options: {
            separator: ';'
         },
         dist: {
            src: ['src/*.js', 'node_modules/jquery/dist/jquery.min.js'],
            dest: 'htdocs/main.min.js'
         }
      }
   } );

   grunt.registerTask( 'default', ['concat'] );
}

